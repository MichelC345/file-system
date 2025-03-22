#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include "LinkedList.h"

struct BootRecord {
    uint16_t bytesPerSector;       // Offset 0x00: Número de bytes por setor (2 bytes)
    uint8_t sectorsPerBlock;       // Offset 0x02: Número de setores por bloco (1 byte)
    uint16_t reservedSectors;      // Offset 0x03: Número de setores reservados (2 bytes)
    uint32_t totalSectors;         // Offset 0x05: Número total de setores (4 bytes)
    uint16_t dirBlockSectors;      // Offset 0x09: Número de setores no Bloco de Diretórios (2 bytes)
    uint32_t dataSectionSectors;   // Offset 0x0B: Número de setores na Seção de Dados (4 bytes)
    uint32_t fileCount;            // Offset 0x0F: Quantidade de arquivos armazenados no sistema (4 bytes)
    LinkedList* freeBlocksList;       // Offset 0x13: Ponteiro para o primeiro bloco livre (4 bytes)
    char empty[1000];            // Offset 0x17: Vazio. Preenche o resto do setor (1000 bytes)
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
    uint32_t block[8];           // Blocos para armazenar dados (8 blocos de 4 bytes cada)
    uint32_t nextIndexBlock;     // Ponteiro para o próximo bloco de índices (4 bytes)
};

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
                    boot.dataSectionSectors = totalSectors - boot.reservedSectors - boot.dirBlockSectors;
                    boot.fileCount = 0;
                    // Inicializa a lista de blocos livres
                    boot.freeBlocksList = new LinkedList();
                    for (uint32_t i = 0; i < totalSectors; i++) {
                        boot.freeBlocksList->insertAtEnd(i);
                    }

                    std::ofstream disk("disk.dat", std::ios::binary | std::ios::trunc);
                    if (!disk) {
                        std::cout << "Erro ao criar o arquivo do disco!\n";
                        break;
                    }

                    // Escreve o BootRecord no início do disco
                    disk.write((char*)&boot, sizeof(BootRecord));

                    // Escreve o Bloco de Diretórios no segundo setor
                    // inicialmente vazio, mas a cada arquivo criado, um novo FileEntry será adicionado
                    char dirBlock[1024] = {0};
                    disk.write(dirBlock, 1024);

                    // Preenche o restante do disco com zeros
                    std::vector<char> emptySector(boot.bytesPerSector, 0);
                    for (uint32_t i = 1; i < totalSectors; ++i) {
                        disk.write(emptySector.data(), boot.bytesPerSector);
                    }

                    disk.close();
                    std::cout << "Partição formatada com sucesso!\n";
                }
                break;
            case '1':
                
                break;
            case '2':
                break;
            case '3':
                break;
            case '4':
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