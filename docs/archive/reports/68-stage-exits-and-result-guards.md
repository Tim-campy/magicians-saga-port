# Estados de saída e guardas do resultado

Data: 2026-10-25. Análise estática do ELF ARM64. VAs sem base Ghidra0x100000; APK não executado.

## Estados recuperados

StageSystem.Update37914 (0x11625e8) e callback ReturnTown37975 (0x116847c) exportados em build/research/stage-exits. Tabela de salto em0x645c36, relativa a0x1162718 em unidades de4 bytes, resolvida em stage-exit-evidence.json:

| Estado | Destino ELF | Ação local relevante |
|---|---|---|
|0|0x1162718|Espera uma condição externa antes de escrever estado1|
|1|0x1162934|CollectDrop, UpdateCollectingDrops, ExecEvent e progressão da fase|
|2|0x11627ec|Tratamento de pausa/retomada e diálogo de saída|
|3|0x1162860|Espera CheckKeyPulse(0x100000), chama ShowQuestResult, depois escreve estado4|
|4|0x11627d0|Chama método da quest, depois ReturnTown(0)|
|5|0x1162958|ReturnTown(1)|
|6|0x1162960|ReturnTown(2)|

No estado1, após coleta e eventos, a combinação isBossDefeated_ (+0x85) e isRewardTreasureBoxPickedUp_ (+0x86) conduz ao estado3. Antes percorre membros e muda estado humano conforme predicado. Não basta derrotar o chefe: a recompensa também precisa ter sido coletada nesse caminho.

O estado3 chama ShowQuestResult em0x11628a8 somente após o pulso de entrada. Depois atualiza equipamento do herói e escreve estado4. O próximo processamento do estado4 retorna à cidade com código0. As chamadas de atualização de equipamento e o método da quest ainda devem ser auditados antes de portar a transição inteira.

## Falha e saída voluntária

Enquanto o chefe não está derrotado, estado1 decrementa remainingTime_ com limite inferior0. Quando chega a0 chama OnGameOver(2). OnHumanDead37939, exportado em result-callers, consulta CharaData.get_isMain38116: se verdadeiro, chama OnGameOver(1). Portanto os códigos1/2 distinguem morte do personagem principal e tempo esgotado neste fluxo.

OnGameOver usa isRetried_: após retry, código2 seleciona estado6; outros selecionam estado5. Antes do retry, abre diálogo. No tratamento desse diálogo, seleção que não segue retry escolhe estado5/6 conforme código armazenado. A opção de retry inclui seus próprios checks e mutações, ainda não portada.

Diálogo identificado por campo+0x240=99 e resposta+0xc8=0 chama ReturnTown(3). Esse diálogo é preparado pelo caminho de pausa/saída no estado2. Os rótulos/textos exatos das opções não foram recuperados; não atribuir nomes de botões por suposição.

ReturnTown(1/2/3) registra UpdateQuestRecord(código,0,0); todos os códigos passam por CleanUp e MemberSystem.OnFinish antes da transição. Esses ramos locais não passam por ShowQuestResult. Isso restringe a entrega descrita em67, mas não prova ausência de outras concessões em callbacks/handlers ainda não examinados.

## Exceção de debug e callback

Varredura de BL/B nos segmentos executáveis encontrou duas chamadas diretas a ShowQuestResult: StageSystem.Update e QuestForm.Update35854, site0x11db0b0. A segunda exige Config.DEBUG (campo estático+0x10), depois entrada por máscara0x20 ou outro teste de tecla. GOT0x272f5c0 resolvido para Config e offsets confirmados via tabela IL2CPP. Portanto não é um segundo fluxo normal comprovado; o valor efetivo de DEBUG ainda depende da configuração. Busca direta não exclui chamadas indiretas.

Callback ReturnTown37975 troca o formulário e envia evento31 com dados derivados de código de saída, id da área e stage. O payload é empacotado em registradores antes de uma chamada genérica; o tipo completo e os assinantes ainda precisam ser resolvidos. Nenhuma entrega de recompensa deve ser adicionada ao callback por inferência.

## Validação e próximo passo

Quatro corpos novos exportados com cobertura sintática, sem alterar C++. Não repetidos testes; último build permanece10/10 em66. A confirmação das guardas muda o plano: integrar resultado apenas no fluxo comprovado e investigar evento31/OnFinish e concessões de equipamento antes de fechar saída da quest. Ainda há pesquisa estática útil.
