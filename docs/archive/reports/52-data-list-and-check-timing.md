# Descritores de dados e preparação das verificações

Data: 2026-10-25. Análise estática do mesmo APK1.4.0 e ELF ARM64 de51. VAs abaixo sem base Ghidra; exports acrescentam0x100000. Nenhuma execução Android ou leitura de save.

## Lista concreta usada por NewData e DeserializeData

AppData.GetDataList34321 (`0x11023fc`,1.872 bytes) cria uma lista de dez descritores, cada um com dois elementos: referência ao array retornado pelo getter e um JInteger construído com constante. O construtor JInteger13370 (`0x1b11454`,40 bytes) guarda o argumento em+0x10 após Object..ctor. Não se trata da posição ordinal do descritor: as constantes são os seletores usados pelo caminho de48 para obter os blocos do SecureProperty.

| Ordem | Getter de Data / método | Tipo de retorno verificado | Seletor |
|---:|---|---|---:|
|0|get_buildingData38332|BuildingData[]|11|
|1|get_charaData38346|CharaData[]|0|
|2|get_areaData38339|AreaData[]|7|
|3|get_magicShopData38348|MagicShopData[]|8|
|4|get_questData38330|QuestData[]|9|
|5|get_equipmentData38334|EquipmentData[]|10|
|6|get_eventData38335|EventData[]|14|
|7|get_hatData38345|HatData[]|15|
|8|get_clothData38333|ClothData[]|16|
|9|get_kingRewardData38343|KingRewardData[]|17|

Os tipos foram resolvidos pelos return_type_index da metadata, tabela nativa de tipos e ponteiros realocados dos elementos SZARRAY; não apenas pelos nomes dos getters. Resultado local: `build/research/data-list-return-types.json`. Os dez getters têm100 bytes cada e retornam campos do objeto apontado pelo primeiro campo estático de Data, após guarda de inicialização de classe. Campos respectivamente+0x20,+0x90,+0x58,+0xa0,+0x10,+0x30,+0x38,+0x88,+0x28,+0x78. Eles não constroem novas instâncias dos registros em seus corpos; conteúdo, quantidade e inicialização do objeto Data ainda requerem investigação.

Isso restringe os overrides de51: nove rotinas concretas de NewGame e o RET herdado por EventData são relevantes para os tipos declarados dessa lista. Não presumir que as outras classes sejam incluídas nesse fluxo. A ordem importa tanto na reinicialização quanto na desserialização.

## InitCheckScriptData e GetTiming

AppData.InitCheckScriptData34265 (`0x1102b4c`,1.060 bytes, export já existente) chama GetDataList, zera inteiro+0x238, cria array de cinco elementos de32 bits em+0x240 e passa dimensões[5,0] a JTool.MakeArray, guardando o resultado em+0x248. A referência genérica no GOT0x272fb28 resolve para método13203, instanciação3905. O helper MakeArray ainda não foi examinado: a forma exata do resultado não deve ser atribuída somente pelo nome.

O laço começa com índice1 e prossegue até o comprimento do resultado+0x248. Para cada índice, percorre os descritores em ordem e os objetos não nulos de seus arrays. Consulta slot14 (class+0x218) e inclui no vetor temporário aqueles cujo retorno é igual ao índice atual. Depois copia os objetos para um BaseData[] e guarda seu comprimento em+0x240 no mesmo índice. O índice0 não é preenchido por esse laço.

As referências genéricas GOT0x272faf8,0x272fb38,0x272fb40 resolvem respectivamente para Vector`1.AddElement13218, ElementAt13225 e Size13244, todas com instanciação de classe28. A associação das chamadas esclarece acumulação e cópia, mas não constitui verificação exaustiva dos contratos internos do vetor.

Slot14 nos22 tipos auditados em51 resolve para BaseData.GetTiming38058 (`0x116c7b0`,8 bytes), que retorna zero incondicionalmente. Portanto os tipos concretos conhecidos não satisfazem a comparação com índices positivos desse laço. Isso é evidência estática de grupos sem elementos para esses tipos, não prova de um mecanismo global de scripts desativado nem justificativa para remover o código. Preservar a distinção entre os valores retornados, a construção dos grupos e os consumidores ainda desconhecidos.

Ao final, o método encaminha AppData+0x2e8 e constante10000 para Util.ArrayInit34558 (`0x117eb08`,68 bytes). Este escreve o argumento em cada elemento de32 bits do array recebido; array nulo leva ao helper de erro. Não aloca o array nem limpa o World nesse corpo. O significado de10000 e os consumidores do array permanecem desconhecidos.

## Validação e limites

Novos exports: `checkscript-context/` (três FDEs,1.948 bytes) e `data-list-getters/` (11 FDEs,1.040 bytes). Todos com cobertura sintática integral e hashes em coverage.tsv. InitCheckScriptData foi relido a partir do export anterior; não é uma nova execução do original. Nenhum C++ alterado ou teste de build repetido.

Os corpos examinados estreitam a preparação pré-carga para dados e tabelas locais; ainda não estabelecem limpeza do ECS. Inicializadores de classe, população dos arrays, MakeArray e consumidores dos campos+0x238/+0x240/+0x248/+0x2e8 são os próximos alvos. O trabalho estático continua disponível; não há necessidade de teste dinâmico agora.
