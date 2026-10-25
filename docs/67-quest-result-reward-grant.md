# Resultado da quest e entrega de recompensas

Data: 2026-10-25. Investigação estática ARM64. Endereços ELF; exports Ghidra usam base adicional0x100000. Nenhum jogo executado ou save acessado.

## Evidência e fluxo confirmado

Exportados StageSystem.ShowQuestResult37917 (0x1164548), ReturnTown37934 (0x1162dc4), CleanUp37936 e OnGameOver37940 em build/research/quest-result. Helpers AddMoney34334, AddCrystal34340, QuestData.AddRescuedPrisoner38510 e CharaData.GetNextExp38133 estão em reward-grant. Cobertura sintática e hashes nos respectivos coverage.tsv; campos de TreasureData em quest-result-fields.json.

ShowQuestResult lê os totais do acumulador da fase e percorre treasureIds em ordem. Para cada TreasureData, soma exp_ (+0x34), money_ (+0x38) e crystal_ (+0x3c) em32 bits aos totais locais. Não limpa nem deduplica a lista nesse trecho. Campos weaponId/charaId/clothId/hatId ocupam+0x40/+0x44/+0x48/+0x4c; os caminhos de desbloqueio/concessão destes itens ainda precisam de auditoria completa.

Após processar os tesouros, chama AppData.AddMoney(totalMoney) em0x1164c04, AddCrystal(totalCrystal) em0x1164c20, QuestData.AddRescuedPrisoner(drop.prisonersNum) em0x1164c3c e UpdateQuestRecord(0,totalExp,totalMoney) em0x1164c50. Isso estabelece entrega além da mera contabilização da coleta. Não comprova em que resultados de fase esse método é alcançado.

## EXP dos personagens

O laço seguinte obtém HumanComponent e guarda level_ (+0x1c) e exp (+0x20) anteriores. Consulta ParameterComponent[0].value; somente resultado>=1 permite somar totalExp ao campo exp, com wrap32. Cada entidade elegível recebe o total completo nesse trecho; não há divisão pelo número de membros no laço observado. Identidade exata da coleção enumerada ainda requer resolução da chamada genérica anterior.

Depois usa um índice iniciado no nível anterior para consultar custos por CharaData.GetNextExp. Se wrap32(exp-custo)<0, encerra; caso contrário grava o restante e incrementa level_ com Clamp(1,1200), avançando também o índice de consulta. A quantidade de entradas de Data.tableData limita o laço. A semântica de progressão não deve ser substituída por um limite de nível isolado: índice, quantidade de tabelas e campo de nível participam separadamente.

GetNextExp, VA0x116d9e4, limita o índice de entrada a[1,1200], lê tableData[indice]+0x24, multiplica pelo campo CharaData+0x9c em32 bits e divide por100 com truncamento para zero. Os nomes desses dois campos e os valores concretos da tabela ainda precisam ser recuperados. Não implementado um custo aproximado.

## Moedas e saída da fase

AddMoney e AddCrystal acessam slots distintos do array em AppData+0x198 → +0x28. Ambos leem o valor por helper, somam o argumento int32 com extensão de sinal usando uma instrução ADD de64 bits e reconvertem pelo helper antes de armazenar. Portanto não reutilizar a soma32 do acumulador para o saldo permanente. Não há clamp explícito no corpo desses métodos; o contrato dos helpers permanece pendente antes de afirmar limites do saldo.

AddRescuedPrisoner soma em32 bits no campo QuestData+0xa0, sem clamp local.

ReturnTown recebe um argumento inteiro. Quando não zero chama UpdateQuestRecord(argumento,0,0); depois chama CleanUp e MemberSystem.OnFinish, e prepara uma transição com callback. Não chama ShowQuestResult diretamente nesse corpo. OnGameOver distingue isRetried_ e código2, seleciona estados5/6 ou um diálogo. Esses números ainda não foram vinculados com segurança a vitória/abandono/derrota; não concluir que todas as recompensas são preservadas ou descartadas em cada saída.

## Estado da implementação

Sem alterações C++ neste ciclo: fechar os callers/estados, inventário e wrappers de saldo é necessário antes de integrar concessão definitiva. A última validação continua sendo10/10 CTest do relatório66; nenhum teste foi repetido sem mudança. Próximo: resolver entrada em ShowQuestResult, callback de ReturnTown e coleção de personagens; então implementar a entrega com a ordem observada.
