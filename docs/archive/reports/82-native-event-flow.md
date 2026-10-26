# Executor nativo de scripts e continuações por contagem

Data: 2026-10-25. Implementação de contratos recuperados estaticamente; não é gameplay completo nem validação dinâmica.

## Evidência e implementação

O módulo `src/core/gameplay/event_flow.{hpp,cpp}` implementa dois trechos:

| Operação | Evidência original | Contrato portado |
| --- | --- | --- |
| execute_event_script | AppData.ExecuteScript34377, VA0x110c8ec; relatório77 | Percorre a matriz a partir da linha indicada, despacha instruções normais e retorna o último resultado. Opcode10 cria continuação e interrompe a matriz. |
| Captura da continuação | DelayEvent35499, VA0x11c9508; relatório78 | Guarda próxima linha, espera, hora opcional e personagem de fala; mantém args por referência e copia argi quando não nulo. |
| process_counted_event_delays | SelectForm.Update35965, VA0x11ec578–0x11ec6d8; relatório78 | Captura contagem inicial, ignora hour diferente de-1, decrementa wait com wrap32, remove antes da retomada e restaura slot7 antes de resolver o script. |

Os objetos de script preservam identidade compartilhada. Os parâmetros são capturados por valor como referências, de forma que substituir a variável do chamador durante um callback não troca o script que já está executando. Null e array vazio permanecem distintos.

O acesso à fila é por índice, com um limite local. Remover a entrada atual desloca as seguintes; acrescentar uma continuação ao fim não aumenta o limite dessa passagem. Mutações feitas pelo host continuam observáveis: se um callback invalidar um índice ainda esperado, o acesso falha, em vez de continuar sobre uma fotografia da fila.

## Fronteira de integração

EventFlowHost exige implementações explícitas de despacho unitário, identificação por referência, resolução do script e leitura/restauração do personagem de fala. Não há efeitos fictícios de moeda, fala ou desbloqueio no módulo. Os callbacks dos testes são instrumentos de observação, não implementações desses sistemas.

O identificador devolvido pelo host é preservado mesmo quando for-1; não se cancela silenciosamente a continuação. A integração de GetDelayId/GetDelayScript continua sob responsabilidade do host, com os contratos e limites documentados em78/81. A substituição dos argumentos sentinela pertence ao dispatcher unitário, não a este executor de matriz.

process_counted_event_delays representa **uma passagem alcançada da fila**, não um frame, segundo ou relógio global. As guardas anteriores de SelectForm descritas em79 devem ser aplicadas pelo chamador. Esta implementação não liga automaticamente a fila ao visualizador ou ao loop de gameplay.

Os vetores representam arrays gerenciados de comprimento fixo dentro de INT32_MAX; essa é uma precondição da integração, não um formato novo de dados. Erros de acesso/null usam exceções padrão C++, preservando o ponto de interrupção e os efeitos anteriores, sem alegar equivalência com as classes/mensagens de exceção do runtime IL2CPP.

## Verificação

Comando: `tools\build-desktop.cmd`. Build Windows concluído; **11/11 testes CTest passaram**. Log local: `build/research/event-flow-build.log`.

O novo teste cobre uma cadeia de efeitos/espera/retomada/novo adiamento, ordem de callbacks, preservação de identidade e resultado, captura de contexto, null versus vazio, hora explícita não decrementada, esperas zero/negativa, wrap INT_MIN→INT_MAX, remoções adjacentes, erros de linha e falha de resolução após remoção. Também verifica alteração das referências do chamador e mutação da fila por callback.

Não houve execução do APK, comparação dinâmica, build Linux ou nova validação do visualizador. Os testes verificam o contrato estático implementado, não provam que um cenário completo do jogo esteja reproduzido.

## Próximo passo

Integrar o host com identificação/resolução de registros e efeitos dos opcodes presentes na tabela, preservando a sequência e as guardas do original. Continuar rastreando hora e SubForm.delayEvent_ como rotas pendentes; não inventar sua ligação ao mecanismo implementado. Este módulo aproxima a execução da progressão do jogo, mas ainda não constitui o primeiro loop jogável.
