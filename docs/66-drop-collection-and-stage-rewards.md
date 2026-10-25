# Coleta e acumulador de recompensas da fase

Data: 2026-10-25. Análise estática do ELF ARM64 inventariado. VAs sem base adicional Ghidra0x100000. Não houve execução do APK.

## Evidência

Exports em build/research/drop-collection: CollectDrop37921 (0x1163738,1248 bytes), UpdateCollectingDrops37922 (0x1163c18,528), OnCollectedDrop37942 (0x1165988,740) e DropComponent.get_isCollecting37299 (0x113edf8,16). Cobertura sintática integral com hashes em coverage.tsv. Campos e tabela de salto registrados em drop-collection-evidence.json.

DropComponent contém type+0x10, value+0x14, x0+0x18, z0+0x1c e frame+0x20. get_isCollecting retorna frame>=0, sem consultar presença numa lista. Acumulador StageSystem.drop_ em+0x40 tem exp+0x10, money+0x14, prisonersNum+0x18 e treasureIds+0x20.

## Início e término da coleta

CollectDrop obtém GetFrontLineHuman37926 e percorre o grupo selecionado pelo componente34. Ignora drops cujo frame já seja não negativo. Para tesouro tipo3 consulta TreasureData.get_isBossReward38579. No caminho de recompensa de chefe, considerando coordenadas finitas, exige frontLine.z<=drop.z e isBossDefeated_ (+0x85) verdadeiro; chama OnCollectedDrop diretamente e marca isRewardTreasureBoxPickedUp_ (+0x86), depois toca jingle. A semântica interna de isBossReward ainda precisa ser examinada.

Nos demais casos, exige frontLine.z<=drop.z+100, grava frame=0 e acrescenta a entidade à lista collectingDrops_ (+0x98). Não concede a recompensa nesse ponto.

UpdateCollectingDrops consulta MemberSystem.get_hero37802 para o alvo de posição. Para cada entrada interpola x/z de x0/z0 até o herói com a sobrecarga float de Easing, duração30, progresso frame e amplitude-80. Depois incrementa frame em32 bits. Se o novo valor >=31, chama OnCollectedDrop e remove a entrada pelo índice, ajustando o índice para não saltar a próxima. Iniciando em0, isso exige31 passagens desse método. Não converter em segundos nem presumir uma passagem por frame de tela antes de verificar o scheduler/callers.

## Contabilização

A tabela de salto em0x645c3d é[0,6,53,59], relativa a0x1165a34 em unidades de4 bytes. OnCollectedDrop lê type/value e faz:

| Tipo | Mutação no acumulador da fase |
|---|---|
|0|exp += value, com wrap32|
|1|money += value, com wrap32|
|2|prisonersNum += 1, ignorando value, com wrap32|
|3|acrescenta value à lista treasureIds, preservando ordem e duplicatas|
|Outros|nenhuma mutação no acumulador|

Após a mutação, tipos0/1 criam efeito numérico. Todos seguem para outro efeito, Entity.AddGarbage(0) e AppData.PlaySe. Tipo desconhecido ainda segue esses efeitos; o módulo nativo abaixo reproduz somente a mutação, não descarta a entidade nem simula o método inteiro. Não há deduplicação local na contabilização. A prevenção de chamadas repetidas depende do fluxo de coleta e remoção.

Isso confirma crédito no acumulador da fase, não transferência definitiva para inventário/EXP dos personagens. Consumidores desse acumulador no encerramento da fase permanecem pendentes, incluindo derrota, abandono e vitória.

## Implementação e verificação

src/core/stage_rewards.hpp/.cpp implementa record_collected_drop sobre estado explícito. Não pressupõe inicialização de uma fase, validade de IDs, saturação, deduplicação ou entrega final ao jogador. std::vector preserva a sequência lógica; não reproduz internals/versionamento/exceções de alocação da lista gerenciada, que ainda não estão integrados a observadores.

tests/test_stage_rewards.cpp verifica sequência mista, separação dos totais, payload ignorado para prisioneiros, IDs duplicados/negativos, tipos desconhecidos e overflow nas três somas. `tools/build-desktop.cmd` passou em Windows com10/10 CTest; log em build/research/stage-rewards-build.log. Não houve comparação dinâmica nem build Linux neste ciclo.

Próximo: seguir o acumulador no encerramento da fase e confirmar guardas/cadência de coleta. O gameplay ainda não é jogável; não implementar concessão permanente por suposição.
