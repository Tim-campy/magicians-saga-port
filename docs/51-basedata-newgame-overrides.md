# Inicialização dos dados antes de Continuar

Data: 2026-10-25. Evidência estática do APK 1.4.0 já identificado em01; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Endereços abaixo são VAs ELF; exports Ghidra somam0x100000. Nenhum APK executado ou save acessado.

## Escopo e método

O fluxo de49 chega a AppData.NewData, que despacha slot18 de BaseData sobre os objetos dos descritores. A caminhada das hierarquias de metadata encontrou22 tipos incluindo BaseData e10 implementações distintas desse slot. Isso inventaria os tipos possíveis; ainda não prova quais objetos GetDataList fornece em cada execução.

Evidências locais: `build/research/basedata-newgame-overrides.json`, `basedata-newgame.tsv` e diretório `basedata-newgame/`. Dez FDEs,924 bytes, cobertura sintática integral. A resolução adicional do slot8 está em `basedata-slot8.json`; quatro callees,632 bytes, em `basedata-reset-callees/`. Cobertura sintática não significa cobertura dinâmica ou alcance de todas as funções.

## Implementações encontradas

| Tipo | Método | VA | Observação delimitada |
|---|---:|---|---|
| AreaData |38031|0x116bf68|Find(true) se bit0 de+0x1c; depois oldNewGame |
| BaseData |38069|0x116bf98|RET; também herdado por12 descendentes |
| BuildingData |38071|0x116c868|Find(true) sob máscara estática; copia+0x48 para+0x20 |
| CharaData |38122|0x116d628|Recria arrays e copia campos; array de três inteiros recebe[0,300,0]; Find(true) condicional |
| ClothData |38182|0x116e458|Find(false) se bit0; copia+0x4c para+0x20 e bit1 para+0x24 |
| EquipmentData |38250|0x116f0c8|Find(false) se bit0; copia+0x80 para+0x28 e bit1 para+0x2c |
| HatData |38387|0x1171894|Find(false) se bit0; copia+0x44 para+0x60 e bit1 para+0x64 |
| KingRewardData |38410|0x1171d64|Zera byte+0x48 |
| MagicShopData |38426|0x1172014|Find(true) sob máscara estática; zera+0x40 |
| QuestData |38507|0x1172e88|Find(true) condicional; zera byte+0xa4; novo QuestRecord em+0xa8 |

Os descendentes que herdam o RET são BulletData, EnemyData, EventData, EventMsgData, ManualData, NewsData, TableData, TalkData, TownData, TreasureData, UnlockShopData e VoiceData. Os offsets são evidência de layout, não nomes inferidos de regras de gameplay. Valores das duas máscaras estáticas e semântica completa dos campos continuam pendentes.

## Find: diferença entre base e quest

O dispatch usa class+0x1b8, isto é, slot8. Em21 tipos examinados resolve para BaseData.Find38052 (`0x116c790`,20 bytes): grava inteiro1 em+0x10 e o bit0 do argumento em byte+0x14. Não há chamada de limpeza nesse corpo.

QuestData usa override38509 (`0x1172f0c`,28 bytes): se inteiro+0x10 já é diferente de zero, retorna sem escrever; caso contrário grava1 no inteiro+0x10 e no byte+0x14. Esse corpo não usa o argumento booleano. Portanto não substituir por uma atribuição genérica incondicional nem interpretar Find como busca sem efeitos.

QuestRecord..ctor35550 (`0x11ce190`,8 bytes) encaminha para System.Object..ctor3955 (`0x1ea3e78`,associação pelo mapa IL2CPP). O corpo do construtor de QuestRecord não contém inicialização explícita do World.

## AreaData.oldNewGame

Método38033 (`0x116bf9c`,576 bytes) aloca estrutura com três linhas de seis elementos de32 bits em+0x20 e três linhas de dois elementos em+0x38; os laços zeram os elementos. Aloca também seis elementos de32 bits em+0x28 e os zera; zera inteiro+0x30. Ao final grava bit0 do campo+0x1c em inteiro+0x10. A identificação dos tipos nativos dos arrays e os nomes funcionais desses campos ainda devem ser recuperados; as dimensões e larguras vêm das instruções.

Assim, a escrita anterior de Find não encerra a análise do estado de AreaData: oldNewGame escreve novamente+0x10. As alocações temporárias visíveis também não devem ser confundidas com objetos armazenados permanentemente.

## Consequências e limites

As implementações e callees examinados mostram reinicializações locais de dados. Não demonstram a limpeza do registro ECS antes de World.Deserialize. A hipótese de mundo inicialmente vazio continua aberta; não adicionar reset arbitrário ao port. Helpers de alocação/runtime, inicializadores estáticos e outros caminhos indiretos não foram exaustivamente analisados.

Próximo recorte: GetDataList para identificar os objetos efetivamente percorridos; InitCheckScriptData e transições anteriores para efeitos adicionais. Há trabalho estático útil, sem necessidade de solicitar teste dinâmico neste ponto. Nenhum C++ alterado; não houve repetição de testes de build por esta mudança documental.
