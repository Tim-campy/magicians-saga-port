# Avanço de páginas e encerramento da fala

Data: 2026-10-25. Capítulo4/M3. Análise estática ARM64; sem execução do APK.

## Pergunta e método

Como o formulário de tipo0 consome os dados preparados em89? Exportados os intervalos FDE de SubForm.Update36149, _update36150, OnTouchEvent36184 e SetEvent36190 com Ghidra headless. A tabela de saltos foi lida dos bytes ELF, evitando atribuir um bloco a um tipo somente pela proximidade textual.

Evidência local: `build/research/form-update.tsv`, `form-update/M36149.asm`, `text-progress.tsv` e `text-progress/*.asm`, todos sob build/research. Endereços abaixo são VAs ELF; os exports Ghidra acrescentam base0x100000. Mesmo ELF ARM64 identificado no inventário. Cobertura sintática dos exports não demonstra execução ou cobertura comportamental.

## Despacho e entrada

SubForm.Update36149 começa em0x11f7a44. Tabela de12 entradas uint16 em0x645ddc: destino =0x11f7dec + entrada×4. Tipo0 resolve0x11f829c; tipo4 resolve0x11f80cc. Portanto o bloco de fala não deve ser aplicado automaticamente às mensagens de evento.

Antes do despacho há MyFormBase.Update35780, contador de encerramento em+0x244, outras guardas e IsDrawWindowEnd36214. O contador não negativo é decrementado; resultado<=0 chama FormBase.Pop19628. Não chamar isso de oito frames renderizados: são passagens de Update sujeitas ao scheduler ainda não integrado.

## Caminho confirmado da fala

1. Em0x11f829c chama TextLayout.Frame19208 no objeto+0x208, antes de testar a entrada.
2. Consulta Canvas.CheckKeyPulse17614 com máscara0x100000. Alternativamente exige flag estático+0xbb do tipo obtido pelo GOT0x272f5c0 e CheckKeyState17612 com máscara0x40. O significado do flag e os nomes físicos das entradas não foram fechados neste ciclo.
3. Se houver avanço, consulta TextLayout.IsEnd19220 (0x1bc370c).
4. Se falso, chama Flush19219 (0x1bc36b0) em0x11f85e8, sem incrementar textIdx.
5. Se verdadeiro, incrementa textIdx+0x210 com aritmética32 e compara com textAry+0x1f8. Havendo próxima página, chama SetText19197; índices inválidos seguem os erros do original.
6. Após a última página, lê nextIndex+0x220. Valor-1 alcança0x11f8a40 e grava8 no contador+0x244; não faz Pop imediato.

IsEnd retorna true quando campo+0x40 é-1 ou quando ele é maior/igual (comparação signed32) ao produto32 de campo+0x34 por1000. Flush grava esse produto em+0x40 e também recalcula o campo temporal+0x48: reduzir Flush a uma mudança booleana perderia efeitos originais.

Frame19208 usa tempo, velocidade, compensação e atualização de blocos internos. Sua fórmula completa e unidades não são declaradas resolvidas aqui.

## Continuação pelo catálogo

Se nextIndex !=-1, o trecho0x11f8854–0x11f89bc:

- Acessa AppData.talks_[nextIndex], aplica substituição se talkArg+0x218 não nulo e faz split TAB.
- Interpreta campo0 com JInteger.ParseInt13380 como personagem. Para-1 consulta o objeto do slot7 de AppData+0x198→+0x20; se ainda-1 usa0. Confere o índice contra o vetor+0x1b0 antes de gravar character+0x200.
- Interpreta campo1 como novo nextIndex.
- Copia os campos2 em diante para textAry, zera textIdx e exige uma primeira página para SetText.

Esse formato de continuação difere da fábrica inicial, que recebe personagem separadamente. Não aplicar a leitura dos dois campos iniciais indiscriminadamente a todos os registros Talk. A fábrica de89 inicializa nextIndex=-1; outros produtores desse campo e rotas que habilitam a continuação ainda precisam ser mapeados.

## Confiança, limites e próximo passo

**confirmed:** destinos do switch, ordem Frame→entrada→IsEnd→Flush/avanço, consulta da continuação e contador de encerramento, conforme instruções e mapa de métodos. **unknown:** equivalência visual, significado completo do flag, temporização, produtores externos de nextIndex e ciclo de atualização integrado.

Nenhuma alteração de C++ neste ciclo; não repetidos os16 testes já aprovados em89. Próximo: resolver produtores de nextIndex e estado temporal de TextLayout antes de portar o avanço; investigar separadamente tipo4. Ainda há investigação estática útil, sem necessidade de teste dinâmico agora.
