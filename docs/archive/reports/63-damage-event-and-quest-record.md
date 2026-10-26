# Evento de dano e recorde de quest

Data:2026-10-25. Gameplay prioritário. Análise estática do mesmo ELF ARM64, sem execução do APK. VAs sem base Ghidra0x100000.

## Ligação do evento15

StageSystem.Init37912 (`0x1162298`,840 bytes) constrói delegate usando referência de método GOT0x2731a18, decodificada como StageSystem.OnDamage37944. No final chama World.AddEventHandler36961 com evento15 e esse delegate; GOT0x27313c0 resolve sua instanciação2565, a mesma usada por World.Send em Damage (62). Isso confirma a inscrição, não apenas semelhança de nome.

OnDamage37944 (`0x11666f8`,64 bytes) exige entidade não nula, consulta Entity.get_hasEnemy36817 e, somente se true, soma o argumento de dano ao campo+0x88 do sistema. Tabela nativa de offsets associa esse campo a totalGaveDamage_. A soma é32 bits, sem clamp e sem recálculo de HP perdido.

Portanto, para esse handler, dano excedente ao HP e chamadas cujo Sub foi pulado por DEBUG ainda podem contribuir com o valor D informado por Damage, sob a guarda de receptor inimigo. Não trocar esse argumento por min(dano,HP anterior) nem por diferença antes/depois. O handler não altera diretamente o parâmetro de HP; essa constatação não exclui outros assinantes do evento15.

## Reset e persistência lógica do recorde

StageSystem.Start37915 zera totalGaveDamage_ no site0x116526c, junto de outros campos de início da fase. Não presumir que toda retomada/Retry passe necessariamente por esse reset sem seguir seus callers.

StageSystem.UpdateQuestRecord37935 (`0x1165728`,156 bytes) obtém QuestData+0xa8, cujo objeto é QuestRecord. Antes da guarda sobre o argumento w1, incrementa challengeCount e soma EnemySystem+0x34 a defeatedCount. Se w1 é diferente de zero, retorna. Se w1 é zero:

- atualiza bestExp e bestMoney somente quando argumentos w2/w3 são maiores;
- incrementa clearCount;
- soma campo+0x18 de StageSystem.drop_ a rescuedCount;
- atualiza bestTotalGaveDamage apenas se totalGaveDamage_ for maior.

Nomes de QuestRecord comprovados pela metadata e tabela de offsets: bestExp+0x10, bestMoney+0x14, challengeCount+0x18, clearCount+0x1c, defeatedCount+0x20, rescuedCount+0x24 e bestTotalGaveDamage+0x28. A interpretação completa do argumento w1 e dos campos de origem de inimigos/drop ainda deve ser ligada aos callers; não implementar sucesso/fracasso baseado só em nomes.

GetBestTotalGaveDamage37958 (`0x116818c`,180 bytes) percorre Data.get_questData, lê record+0x28 e mantém máximo assinado começando em0. Não soma recordes nem consulta apenas quest atual. Elementos/records null chegam a helper de erro; não são ignorados nesse corpo.

## Evidência e limitações

Exports `stage-damage/`: Init, OnDamage e construtor, cobertura sintática integral. `stage-damage-total/`: Start, UpdateQuestRecord e GetBestTotalGaveDamage. Busca auxiliar de LDR/STR32 com offset0x88 em métodos StageSystem registrada em `stage-damage-fields.json`; cobertura limitada a esse modo de endereçamento/tipo, não prova de todos os escritores.

Nenhum C++ alterado ou teste repetido. A investigação liga agora um evento do combate ao registro de progresso. Próximo: outros assinantes e callers de UpdateQuestRecord, sem perder o fechamento numérico de Easing necessário à integração de Damage. Ainda não há port jogável ou validação dinâmica.
