# Indexação dos dados pelo ID

Data: 2026-10-25. Análise estática ARM64. VAs ELF, sem base Ghidra0x100000. Nenhuma execução do jogo.

## Ligação ao array de eventos

Data.Load38354 em0x1170974 seleciona o endereço de destino e uma instanciação genérica para cada entrada. No ramo com destino Data+0x38, o GOT0x2731db0 contém usage0xc001c065. A especificação decodificada aponta para método38355 LoadData, com instanciação3820. Esta contém tipo EventData, confirmado via tabela de tipos genéricos.

O ramo chama o corpo compartilhado LoadData em0x129fbe8 e grava o array retornado em+0x38. Export em build/research/data-generic-load. Esse corpo transforma a sequência recebida por um delegate, materializa um array e passa ao helper de indexação em0x129fafc. O corpo de transformação genérica também foi exportado em data-load-helpers; o contrato completo do delegate e da origem da sequência permanece pendente.

## Algoritmo do helper

O helper0x129fafc percorre os objetos de entrada lendo id_+0x18, com máximo inicial0. Aloca um novo array de tamanho wrap32(maxId+1); entrada vazia aloca tamanho1. Depois percorre novamente os objetos e grava cada referência em destino[id].

- Lacunas ficam null, conforme array recém-alocado.
- IDs duplicados são sobrescritos pela última ocorrência, preservando a ordem da sequência na escolha.
- IDs negativos falham na checagem de índice; não são descartados.
- Objetos nulos falham ao serem desreferenciados; não são removidos silenciosamente.
- Overflow de tamanho não foi convertido numa política de recuperação; o comportamento de erro da alocação ainda pertence ao runtime original.

Isso distingue quantidade de linhas, maior ID e tamanho do array. Não preencher lacunas com registros padrão nem compactar os IDs no port.

## Aplicação aos arquivos locais

Modelo desse algoritmo aplicado às linhas dos dois Event.txt extraídos produziu tamanho229, lacuna no índice180 e nenhum ID duplicado. Resultado/hash/lacunas em build/research/event-index-model.json. O índice1133 fica fora desse resultado e seria ignorado pela guarda de ExecEvent descrita em72.

Essa conclusão é condicional à sequência carregada ser a dessas linhas: ainda falta fechar seleção do recurso, leitura das linhas e delegate de construção. Não foi observado o array em runtime nem provada ausência de substituições posteriores. A chamada1133 não é motivo para inventar um evento ou aumentar o array artificialmente.

## Estado e próximos passos

Novos exports: LoadData compartilhado e dois helpers, com cobertura sintática/hash. Nenhuma mudança C++ nem repetição de testes; último build continua10/10 do relatório70. Próximo: seguir a origem da sequência e a criação de cada objeto para implementar um loader de dados de gameplay com IDs e lacunas preservados.
