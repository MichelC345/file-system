# file-system
Implementação de um sistema de arquivos utilizando a técnica de alocação indexada e gerenciamento de espaço livre com lista ligada. Trabalho da disciplina de Sistemas Operacionais.

BootRecord:
- Número de bytes por setor: 1024
- Números de setores por cluster: 1
- Setores reservados
- Total de clusters
- Número de setores no bloco de diretórios
- Número de setores no bloco de dados
- Número de setores no bloco de gerenciamento de espaço Livre
- Ponteiro para o primeiro cluster livre
- Números de clusters livres
- Cluster inicial do diretório raiz

Bloco de diretórios:
- Contém informações sobre os arquivos e subdiretórios
- Nome do arquivo / subdiretório
- Tipo
- Tamanho
- Cluster inicial (índice)
- Atributos

Subdiretório:
- Um arquivo especial que armazena uma lista de entradas (arquivos e outros subdiretórios)
