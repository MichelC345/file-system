
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <cmath>
#include "LinkedList.h"

using namespace std;

#pragma pack(push, 1)
struct BootRecord {
    uint16_t bytesPerSector;       // Offset 0x00: Número de bytes por setor (2 bytes)
    uint8_t sectorsPerBlock;       // Offset 0x02: Número de setores por bloco (1 byte)
    uint16_t reservedSectors;      // Offset 0x03: Número de setores reservados (2 bytes)
    uint32_t totalSectors;         // Offset 0x05: Número total de setores (4 bytes)
    uint16_t dirBlockSectors;      // Offset 0x09: Número de setores no Bloco de Diretórios (2 bytes)
    uint32_t dataSectionSectors;   // Offset 0x0B: Número de setores na Seção de Dados (4 bytes)
    uint32_t fileCount;            // Offset 0x0F: Quantidade de arquivos armazenados no sistema (4 bytes)
    uint32_t freeBlocksCount;
    uint32_t freeBlocksList;       // Offset 0x13: Ponteiro para o primeiro bloco livre (4 bytes)
    char empty[997];            // Offset 0x17: Vazio. Preenche o resto do setor (1000 bytes)
};

struct FileEntry {
    uint8_t status;               // Offset 0x00: Status. Indica se o arquivo foi apagado (0xFF) ou não (0x00)
    char fileName[16];            // Offset 0x01: Nome do arquivo (16 bytes)
    char extension[4];            // Offset 0x11: Extensão (4 bytes)
    uint32_t fileSize;            // Offset 0x15: Tamanho do arquivo (4 bytes)
    uint32_t indexBlockPointer;   // Offset 0x19: Ponteiro para o bloco de índices (4 bytes)
    uint8_t attributes;           // Offset 0x1D: Atributos (1 byte)
    uint16_t empty;               // Offset 0x1E: Vazio (2 bytes)
};

struct IndexBlock {
    uint32_t block[255];           // Blocos para armazenar dados (255 blocos de 4 bytes cada)
    uint32_t nextIndexBlock;     // Ponteiro para o próximo bloco de índices (4 bytes)
};
#pragma pack(pop)

void displayMenu() {
    std::cout << "Sistema de Arquivos\n";
    std::cout << "0 - Formatar uma partição\n";
    std::cout << "1 - Cópia para o sistema de arquivos\n";
    std::cout << "2 - Cópia de um arquivo para o disco rígido\n";
    std::cout << "3 - Listar arquivos\n";
    std::cout << "4 - Remover um arquivo\n";
    std::cout << "5 - Encerrar\n";
    std::cout << "\nSelecione uma opção: ";
}

uint32_t getNextFreeBlock(fstream& disk, uint32_t currentBlock) {
    uint32_t nextBlock;
    disk.seekg(currentBlock * 1024);
    disk.read((char*)&nextBlock, sizeof(uint32_t));
    return nextBlock;
}

void insertAtEnd(fstream& disk, uint32_t value) {
    BootRecord boot;
    disk.seekg(0);
    disk.read((char*)&boot, sizeof(BootRecord));
    // Acessa a cabeça da lista de blocos livres
    uint32_t freeBlock = boot.freeBlocksList;
    // Vai até a respectiva posição no disco
    disk.seekp(freeBlock * 1024);
    while (true) {
        // Percorre a lista até achar o último bloco
        uint32_t nextBlock;
        disk.read((char*)&nextBlock, sizeof(uint32_t));
        if (nextBlock == 0xFFFFFFFF) {
            // Insere o novo bloco no final
            disk.write((char*)&value, sizeof(uint32_t));
            break;
        }
        disk.seekg(nextBlock * 1024);
    }
    disk.seekp(0);
    disk.write((char*)&boot, sizeof(BootRecord));
    boot.freeBlocksCount++;
    cout << "novo bloco livre " << boot.freeBlocksCount << endl;
} 

void importFile() {
    fstream disk("disk.img", ios::binary | ios::in | ios::out);
    if (!disk.is_open()) {
        cerr << "Não foi possível acessar o arquivo da imagem FAT!" << endl;
        return;
    }
    BootRecord boot;
    disk.read((char*)&boot, sizeof(BootRecord));

    // 2. Verificar se há espaço livre suficiente
    string fileName;
    cout << "Caminho do arquivo local: ";
    cin >> fileName;

    if (fileName.size() > 16) {
        cout << "Nome do arquivo muito longo! Deve ter no máximo 16 caracteres.\n";
        return;
    }
    
    ifstream sourceFile(fileName, ios::binary | ios::ate);

    if (!sourceFile) {
        cout << "Erro ao abrir o arquivo de origem.\n";
        return;
    }

    sourceFile.seekg(0, ios::end);
    uint32_t fileSize = sourceFile.tellg();
    sourceFile.seekg(0, ios::beg);
    uint32_t blockSize = boot.bytesPerSector;
    uint32_t dataBlocksNeeded = (fileSize + boot.bytesPerSector - 1) / boot.bytesPerSector;
    uint32_t indice = (dataBlocksNeeded + 254) / 255;

    // Calcular blocos necessários: 1 (índice) + ceil(fileSize / 1024)
    int blocksNeeded = indice + dataBlocksNeeded;
    if (boot.freeBlocksCount < blocksNeeded) {
        cout << "Espaço insuficiente! Necessário: " << blocksNeeded << " blocos\n";
        return;
    }
    size_t ind = fileName.find(".");
    string name = fileName.substr(0, ind);
    string ext = fileName.substr(ind + 1);
    // 3. Verificar se o arquivo já existe no diretório
    FileEntry entry;
    bool exists = false;
    disk.seekg(1024); // Diretório no setor 1
    for (int i = 0; i < 32; i++) {
        disk.read((char*)&entry, sizeof(FileEntry));
        if (entry.status != 0xFF && strcmp(entry.fileName, name.c_str()) == 0) {
            exists = true;
            break;
        }
    }
    if (exists) {
        cout << "Arquivo já existe!\n";
        return;
    }

    // Alocar blocos de índice
    vector<uint32_t> indexBlocks;
    for (int i = 0; i < indice; i++) {
        uint32_t indexBlock = boot.freeBlocksList;
        boot.freeBlocksList = getNextFreeBlock(disk, indexBlock);
        boot.freeBlocksCount--;
        indexBlocks.push_back(indexBlock);
    }

    // Alocar blocos de dados
    vector<uint32_t> dataBlocks;
    for (int i = 0; i < dataBlocksNeeded; i++) {
        uint32_t dataBlock = boot.freeBlocksList;
        boot.freeBlocksList = getNextFreeBlock(disk, dataBlock);
        boot.freeBlocksCount--;
        dataBlocks.push_back(dataBlock);
    }
    FileEntry newEntry;
    newEntry.status = 0x01; // Válido
    strncpy(newEntry.fileName, name.c_str(), 16);
    strncpy(newEntry.extension, ext.c_str(), 4);
    newEntry.fileSize = fileSize;
    newEntry.indexBlockPointer = indexBlocks[0];
    newEntry.attributes = 0x01;

    disk.seekg(1024); // Posição inicial do diretório
    int dirEntryPos = -1;
    for (int i = 0; i < 32; i++) {
        streampos pos = 1024 + i * sizeof(FileEntry);
        disk.seekg(pos);
        disk.read((char*)&entry, sizeof(FileEntry));
        if (entry.status == 0x00 || entry.status == 0xFF) {
            dirEntryPos = i;
            break;
        }
    }

    if (dirEntryPos == -1) {
        cout << "Não há espaço no diretório para novas entradas." << endl;
        return;
    }

    streampos pos = 1024 + dirEntryPos * sizeof(FileEntry);
    disk.seekp(pos);
    disk.write((char*)&newEntry, sizeof(FileEntry));

    // 5. Alocar blocos de dados e escrever conteúdo
    char buffer[1024];
    uint32_t bytesRemaining = fileSize;

    for (uint32_t i = 0; i < dataBlocks.size(); i++) {
        uint32_t bytesToRead = min(bytesRemaining, static_cast<uint32_t>(1024));
        sourceFile.read(buffer, bytesToRead);

        // Preencha o restante do buffer com zeros se necessário
        memset(buffer + bytesToRead, 0, 1024 - bytesToRead);

        disk.seekp(dataBlocks[i] * boot.bytesPerSector);
        disk.write(buffer, 1024);
        bytesRemaining -= bytesToRead;
    }

    for (int i = 0; i < indexBlocks.size(); i++){

    }

    size_t totalDataBlocks = dataBlocks.size();
    size_t totalIndexBlocks = indexBlocks.size();
    size_t indexPerBlock = 255;  // Número de índices por bloco

    for (size_t i = 0; i < totalIndexBlocks; i++) {
        IndexBlock idxBlock = {}; // Zera a estrutura
        // Preencher o bloco de índices com os ponteiros para os dados
        for (size_t j = 0; j < indexPerBlock; j++) {
            size_t dataIndex = i * indexPerBlock + j;
            if (dataIndex < totalDataBlocks) {
                idxBlock.block[j] = dataBlocks[dataIndex];
            } else {
                // Se não houver mais blocos de dados, preenche com 0 ou outro valor padrão
                idxBlock.block[j] = 0xFFFFFFFF;
            }
        }
        // Se houver um próximo bloco de índice, encadeia
        if (i < totalIndexBlocks - 1) {
            idxBlock.nextIndexBlock = indexBlocks[i + 1];
        } else {
            idxBlock.nextIndexBlock = 0xFFFFFFFF;  // Último bloco de índice
        }
        // Gravar o bloco de índice no disco na posição correspondente
        disk.seekp(indexBlocks[i] * boot.bytesPerSector);
        disk.write((char*)&idxBlock, sizeof(IndexBlock));
    }

    // 8. Atualizar BootRecord
    disk.seekp(0);
    disk.write((char*)&boot, sizeof(BootRecord));
    cout << "Arquivo copiado com sucesso!\n";
    cout << "Blocos livres: " << boot.freeBlocksCount << endl;
}

void listFiles() {
    ifstream disk("disk.img", ios::binary);
    if (!disk) {
        cout << "Erro ao abrir o disco." << endl;
        return;
    }
    disk.seekg(1024);
    FileEntry entry;
    //cout << "Lista de Arquivos:" << endl;
    cout << "\nArquivos no sistema:\n";
    cout << "----------------------------------------\n";
    cout << "Nome \t\t| Extensão \t| Tamanho (KB)\n";
    cout << "----------------------------------------\n";

    for (int i = 0; i < 32; i++) {  // Se o diretório tiver 32 entradas
        disk.read((char*)&entry, sizeof(FileEntry));
        if (entry.status == 0x01) {
            string fileName(entry.fileName, 16);
            fileName = fileName.c_str(); // Remove padding
            string ext(entry.extension, 4);
            ext = ext.c_str();
            cout << fileName
                 << " \t| " << ext
                 << " \t| " << (entry.fileSize / 1024.0) << " KB\n";
        }
    }
}

void exportFile() {
    fstream disk("disk.img", ios::binary | ios::in);
    if (!disk) {
        cout << "Erro ao abrir o disco." << endl;
        return;
    }
    
    string targetFileName;
    cout << "Informe o nome do arquivo a ser copiado: ";
    cin >> targetFileName;
    
    size_t ind = targetFileName.find(".");
    string name = targetFileName.substr(0, ind);
    string ext = targetFileName.substr(ind + 1);

    disk.seekg(1024);
    FileEntry entry;
    bool found = false;
    for (int i = 0; i < 32; i++) {
        disk.read((char*)&entry, sizeof(FileEntry));
        if (entry.status == 0x01 && strcmp(entry.fileName, name.c_str()) == 0 && strcmp(entry.extension, ext.c_str()) == 0) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        cout << "Arquivo não encontrado." << endl;
        return;
    }
    
    // Preparar buffer para receber os dados
    uint32_t totalBytes = entry.fileSize;
    vector<char> fileData;
    fileData.reserve(totalBytes);
    
    // Ler o bloco de índice
    uint32_t indexBlockSector = entry.indexBlockPointer;
    while (indexBlockSector != 0xFFFFFFFF) {
        // Posicionar no bloco de índice e ler a estrutura
        disk.seekg(indexBlockSector * 1024);
        IndexBlock idxBlock;
        disk.read((char*)&idxBlock, sizeof(IndexBlock));
        
        // Para cada ponteiro no bloco de índice (255 entradas)
        for (int j = 0; j < 255; j++) {
            uint32_t dataBlockSector = idxBlock.block[j];
            
            if (dataBlockSector == 0xFFFFFFFF || fileData.size() >= totalBytes)
                break;
            // Ler o bloco de dados
            vector<char> data(1024);
            disk.seekg(dataBlockSector * 1024);
            disk.read(data.data(), 1024);
            
            // Acrescentar os dados lidos ao vetor fileData
            // Se este for o último bloco, pode haver dados a menos que 1024 bytes
            size_t bytesRestantes = totalBytes - fileData.size();
            size_t bytesToCopy = min((size_t)1024, bytesRestantes);
            fileData.insert(fileData.end(), data.begin(), data.begin() + bytesToCopy);
        }
        // Passa para o próximo bloco de índice, se houver
        indexBlockSector = idxBlock.nextIndexBlock;
    }
    
    // Gravar o conteúdo lido em um arquivo no sistema operacional
    ofstream outFile(name + "_copy." + ext, ios::binary);
    if (!outFile) {
        cout << "Erro ao criar o arquivo de saída." << endl;
        return;
    }
    outFile.write(fileData.data(), fileData.size());
    outFile.close();
    
    cout << "Arquivo copiado para o host com sucesso." << endl;
}

void removeFile() {
    fstream disk("disk.img", ios::binary | ios::in | ios::out);
    if (!disk) {
        cout << "Erro ao abrir o disco." << endl;
        return;
    }
    
    // Recebe o nome do arquivo
    string targetFileName;
    cout << "Informe o nome do arquivo a ser removido: ";
    cin >> targetFileName;
    
    // Separa o nome e a extensão
    size_t ind = targetFileName.find(".");
    string name = targetFileName.substr(0, ind);
    string ext = targetFileName.substr(ind + 1);

    // Acessa o bloco de diretórios
    disk.seekg(1024);
    // Procura a entrada correspondente ao arquivo
    FileEntry entry;
    uint32_t entryPos = -1;
    for (int i = 0; i < 32; i++) {
        disk.read((char*)&entry, sizeof(FileEntry));
        if (entry.status == 0x01 && strcmp(entry.fileName, name.c_str()) == 0 && strcmp(entry.extension, ext.c_str()) == 0) {
            entryPos = i;
            break;
        }
    }
    
    if (entryPos == -1) {
        cout << "Arquivo não encontrado." << endl;
        return;
    }
    
    // Desaloca o espaço alocado
    cout << entry.fileName << "." << entry.extension << " encontrado." << endl;
    uint32_t indexBlockSector = entry.indexBlockPointer;
    while (indexBlockSector != 0xFFFFFFFF) {
        uint32_t indexBlockSectorBytes = indexBlockSector * 1024;
        // Posiciona no bloco de índice e lê a estrutura
        disk.seekg(indexBlockSectorBytes);
        IndexBlock idxBlock;
        disk.read((char*)&idxBlock, sizeof(IndexBlock));
        
        // Para cada ponteiro no bloco de índice (255 entradas)
        for (int j = 0; j < 255; j++) {
            uint32_t dataBlockSector = idxBlock.block[j];
            
            if (dataBlockSector == 0xFFFFFFFF)
                break;

            // Adiciona o bloco de dados à lista de blocos livres
            insertAtEnd(disk, dataBlockSector);
            
            // Esvazia o bloco de dados
            disk.seekp(dataBlockSector * 1024);
            std::vector<char> buffer(1024, static_cast<char>(0xFF));
            disk.write(buffer.data(), buffer.size());
        }

        // Adiciona o bloco de índice à lista de blocos livres
        insertAtEnd(disk, indexBlockSector);
        // Esvazia o bloco de índice
        disk.seekp(indexBlockSectorBytes);
        std::vector<char> buffer(1024, static_cast<char>(0xFF));
        disk.write(buffer.data(), buffer.size());

        // Passa para o próximo bloco de índice, se houver
        uint32_t nextIndexBlock = idxBlock.nextIndexBlock;
        disk.seekp(indexBlockSectorBytes + 255 * sizeof(uint32_t));
        disk.write((char*)&nextIndexBlock, sizeof(uint32_t));
        indexBlockSector = nextIndexBlock;
    }

    // Marca a entrada como apagada
    //entry.status = 0xFF;
    //disk.seekg(1024 + entryPos * sizeof(FileEntry));
    //disk.read((char*)&entry, sizeof(FileEntry));
    //cout << "Ainda está aqui " << entry.fileName << "." << entry.extension << endl;
    disk.seekp(1024 + entryPos * sizeof(FileEntry));
    vector<char> buffer(sizeof(FileEntry), 0xFF);
    disk.write(buffer.data(), buffer.size());
    //disk.write((char*)&entry, sizeof(FileEntry));
    
    // Atualiza o BootRecord
    BootRecord boot;
    cout << "Quantidade de blocos livres antes: " << boot.freeBlocksCount << endl;
    disk.seekg(0);
    disk.read((char*)&boot, sizeof(BootRecord));
    //boot.freeBlocksCount += ceil(entry.fileSize / 1024.0); já feito no processo de inserção
    disk.seekp(0);
    disk.write((char*)&boot, sizeof(BootRecord));
    
    cout << "Quantidade de blocos livres depois: " << boot.freeBlocksCount << endl;
    cout << "Arquivo removido com sucesso." << endl;
}

int main() {
    char op;
    uint32_t totalSectors;
    std::ifstream file;
    std::ofstream fileOut;
    while (true) {
        displayMenu();
        std::cin >> op;
        switch (op) {
            case '0':
                {
                    std::cout << "Informe a quantidade de setores para formatar (mínimo 5, máximo 262144): ";
                    std::cin >> totalSectors;

                    if (totalSectors < 5 || totalSectors > 262144) {
                        std::cout << "Quantidade de setores inválida! Deve estar entre 5 e 262144.\n";
                        break;
                    }

                    BootRecord boot = {};
                    boot.bytesPerSector = 1024; // 1024 bytes por setor
                    boot.sectorsPerBlock = 1;   // Exemplo: 1 setor por bloco
                    boot.reservedSectors = 1;   // 1 setor reservado
                    boot.totalSectors = totalSectors;
                    boot.dirBlockSectors = 1;   // 1 setor para o bloco de diretórios
                    boot.dataSectionSectors = totalSectors - (boot.reservedSectors + boot.dirBlockSectors);
                    boot.freeBlocksCount = boot.dataSectionSectors;
                    boot.fileCount = 0;
                    boot.freeBlocksList = boot.reservedSectors + boot.dirBlockSectors;
                    // Inicializa a lista de blocos livres
                    //boot.freeBlocksList = new LinkedList();
                    //for (uint32_t i = 2; i <= totalSectors; i++) {
                        //boot.freeBlocksList->insertAtEnd(i);
                    //}



                    ofstream disk("disk.img", ios::binary | ios::trunc);
                    if (!disk) {
                        cout << "Erro ao criar o arquivo do disco!\n";
                        break;
                    }

                    // Escreve o BootRecord_t no início do disco
                    disk.write((char*)&boot, sizeof(BootRecord));

                    // Escreve o Bloco de Diretórios no segundo setor
                    // inicialmente vazio, mas a cada arquivo criado, um novo FileEntry_t será adicionado
                    char dirBlock[boot.bytesPerSector] = {};
                    disk.write(dirBlock, boot.bytesPerSector);

                    // Preenche o restante do disco com zeros
                    /*vector<char> emptySector(boot.bytesPerSector, 0);
                    for (uint32_t i = 2; i < totalSectors; ++i) {
                        disk.write(emptySector.data(), boot.bytesPerSector);
                    }*/

                    for (uint32_t sector = 2; sector < totalSectors; sector++) {
                        uint32_t nextSector = (sector < boot.totalSectors - 1) ? sector + 1 : 0xFFFFFFFF;
                        // Calcule a posição no arquivo "disk.img" para o setor atual
                        streampos pos = sector * boot.bytesPerSector;
                        disk.seekp(pos);
                        // Escreva o valor de nextSector nos primeiros 4 bytes
                        disk.write(reinterpret_cast<char*>(&nextSector), sizeof(uint32_t));
                    }

                    disk.close();
                    cout << "Blocos livres " << boot.freeBlocksCount << endl;
                    cout << "Partição formatada com sucesso!\n";
                }
                break;
            case '1':
                importFile();
                break;
            case '2':
                exportFile();
                break;
            case '3':
                listFiles();
                break;
            case '4':
                removeFile();
                break;
            case '5':
                return 0;
            default:
                std::cout << "Opção inválida!\n";
        }
    }
    displayMenu();
    return 0;
}
