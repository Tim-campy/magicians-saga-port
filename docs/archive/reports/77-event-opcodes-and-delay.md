# Instruções usadas nos eventos e continuação adiada

Data: 2026-10-25. Análise estática ARM64, sem execução do jogo. VAs sem base Ghidra0x100000.

## Cobertura dos dados

As208 linhas de execs dos eventos extraídos usam oito opcodes:15(74),8(57),10(37),12(22),4(8),2(6),1(3),31(1). Contagens em build/research/event-opcode-usage.json. Essa amostra não cobre necessariamente scripts de quests, termos, eventos construídos ou outros recursos.

Exports das três sobrecargas ExecuteScript em build/research/event-executor. O executor da matriz34377 em0x110c8ec percorre linhas a partir do índice fornecido. Mantém o retorno da última instrução comum; não interrompe a lista simplesmente porque essa instrução retornou um objeto. Linhas null/vazias atingem caminhos de erro neste laço.

## Opcode10 é controle do executor da lista

Ao encontrar10, o laço chama GetDelayId com a matriz completa, lê dados auxiliares do estado, constrói DelayEvent com índice de continuação atual+1 e argumento da instrução, acrescenta-o à coleção AppData+0x200 e retorna sem processar as linhas seguintes. Se existir terceiro inteiro, grava-o em DelayEvent+0x1c; o default passado ao construtor é-1. Campos/temporização devem ser reconciliados com o codec de DelayEvent investigado em55 antes de implementar retomada.

O dispatcher unitário34379 em0x110cc4c não executa o mesmo agendamento para10: sua entrada na tabela de salto retorna pelo caminho comum. Portanto não substituir o executor de matriz por um simples loop que sempre chama o dispatcher unitário. A sobrecarga34378 em0x110d3b0 encaminha ao dispatcher unitário.

## Dispatcher e instruções presentes

Antes de selecionar o opcode, o dispatcher copia a linha e resolve argumentos especiais no intervalo INT_MIN..INT_MIN+99 por índices de um array de parâmetros; a linha original não é sobrescrita. Config.DEBUG acrescenta diagnóstico, mas a checagem de acesso permanece. Não interpretar todo inteiro negativo como valor literal: esses sentinelas são substituições de argumento.

Tabela de salto em0x627b6e,15 entradas uint16 relativas a0x110cee0 em unidades de4 bytes:

|Opcode usado|Destino ELF|Operação observada|
|1|0x110cee0|Valor não negativo chama AddMoney; negativo é negado em32 bits e encaminhado ao método de subtração|
|2|0x110cfdc|Mesmo padrão para cristais|
|4|0x110cff8|Consulta Data.areaData[id], testa isFound e, se necessário, invoca Find(true)|
|8|0x110d070|Chama SubForm.MakeTalkForm com argumento e contexto; encaminha formulário ao helper de apresentação|
|12|0x110d100|Chama SubForm.MakeEvent com argumento/contexto; encaminha ao mesmo helper|
|15|0x110d258|Consulta Data.questData[id] pelo getter38330 em0x116a670, testa isFound e invoca Find(true) quando necessário|
|31|ramo separado0x110cefc|Calcula ajuste percentual baseado em GetStaminaMax e altera um slot de estado por SecureLong, com limites; não é opcode de dano|

Find aqui é virtual. Não reutilizar automaticamente o corpo BaseData.Find de70 sem conferir os tipos AreaData e QuestData. Os ramos negativos chamam SubMoney34336/SubCrystal34341. GetStaminaMax, esses métodos de subtração e o helper de apresentação ainda precisam de análise antes de portar efeitos completos.

## Estado e próximos passos

O ciclo anterior foi progresso ao interpretar os dados. Este identificou o conjunto efetivamente usado, dispatch e diferença essencial do opcode10. Nenhuma alteração C++ nem repetição de testes: ainda falta scheduler de DelayEvent, semântica dos efeitos e construção do loader. Próximo: resolver getters/overrides e consumidor de DelayEvent para ligar eventos, gameplay e persistência sem execução aproximada.
