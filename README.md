# file-system
Implementação de um sistema de arquivos utilizando a técnica de alocação indexada e gerenciamento de espaço livre com lista ligada. Trabalho da disciplina de Sistemas Operacionais.
Cada setor terá 1024 bytes, um setor por cluster.

BootRecord:
- Número de bytes por setor: 2 bytes
- Números de setores por cluster: 1 byte
- Setores reservados: 2 bytes (inclui o BootRecord)
- Total de setores: 4 bytes
- Número de setores no bloco de diretórios: 2 bytes
- Número de setores no bloco de dados: 2 bytes
- Ponteiro para o primeiro cluster livre: 4 bytes
- Números de setores livres: 4 bytes
- Cluster inicial do diretório raiz: 4 bytes
- Vazio (preenchido com 0x00)

Bloco de diretórios:
- Contém informações sobre os arquivos e subdiretórios
- Status: 1 byte
- Nome do arquivo / subdiretório: 16 bytes
- Extensão: 4 bytes
- Tamanho: 4 bytes
- Setor inicial (índice): 4 bytes
- Atributos: 1 byte
- Vazio (preenchido com 0x00)

Subdiretório (Opcional):
- Um arquivo especial que armazena uma lista de entradas (arquivos e outros subdiretórios)
- Entradas especiais são criados com a mesma estrutura de um arquivo, com nomes . e .., não possuem extensão
- Similar ao FAT16
- . tem como cluster inicial o próprio diretório
- .. aponta para o diretório pai (exceto no diretório raiz)
