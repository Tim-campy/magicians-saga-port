# Controle nativo de ajuste dos blocos de texto

Data: 2026-10-25. Capítulo 4/M3. Contrato derivado de análise estática; sem comparação com execução Android.

## Regra recuperada

TextLayout.SetText19198 (0x1bbcfdc), trecho0x1bbd32c..0x1bbd420, executa ajuste após a primeira construção de blocos e uma cópia da fonte. O ajuste exige autoScale_ e height>0, inclusive na rota de linha única.

Na rota multiline, consulta GetHeight19203 (0x1bbfd2c). Enquanto a altura medida ultrapassa height+1, lê Font.size_+0x1c, subtrai1 em32 bits, chama SetSize com usePreset=false e reconstrói os blocos por _makeTextBlocks19200. Sai do laço quando o **tamanho solicitado** foi1, mesmo que o setter tenha produzido outro tamanho efetivo. Caso contrário, relê a altura do layout e mede novamente.

Após o laço, compara height com nova chamada GetHeight. Se ainda há excesso, faz outra chamada GetHeight e grava seu resultado em height. Assim, tolerar um pixel pode evitar reduzir a fonte, mas ainda provocar aumento da altura do layout. As consultas repetidas não foram fundidas no port.

Na rota de linha única, exige também width>0 e reduz a fonte enquanto width<textWidth_+0x68. Cada redução reconstrói os blocos; não usa GetHeight nessa rota. O limite também é o tamanho solicitado1, não um clamp genérico para qualquer tamanho<=1. Não foi adicionado limite arbitrário de iterações.

Comparações FCMP/B.PL e FCMP/B.LE foram traduzidas para comparações ordenadas de float; NaN não entra no ajuste através das guardas positivas. Subtração de tamanho preserva wrap32 sem overflow assinado indefinido em C++.

## Implementação

`src/core/gameplay/text_fit.hpp/.cpp` contém TextFitState e fit_text_blocks. TextFitHost fornece tamanho/setter, reconstrução de blocos e altura medida. O setter representa Font.SetSize(size,false); não substitui a medição do original por uma estimativa.

O chamador deve executar a construção inicial e o snapshot da fonte antes desse contrato. No original, SetText depois calcula comprimento de revelação, chama _makeTextRubies19201 (0x1bbfde8) e restaura font_ com a cópia salva em0x1bbd498. Essas etapas não pertencem a fit_text_blocks e ainda não estão integradas num parser completo. Não tratar esse módulo como TextLayout completo.

Antes da primeira construção, SetText tem ainda uma correção condicionada por adjustFontSize_/multiLine_: duas multiplicações por100 e duas divisões pelo Font.requestFontScale_ estático+0x44, seguidas por SetSize(false). Esse trecho0x1bbd298..0x1bbd300 foi observado, mas não portado neste ciclo; origem/semântica efetiva do fator e interação com localização permanecem abertas.

## Validação

`tools\build-desktop.cmd`, Windows/MSVC14.51: **19/19 CTests passaram**, log `build/research/text-fit-build.log`. A primeira compilação detectou uma chave final de namespace ausente; corrigida antes do build final.

Novo teste text_fit_contract cobre reconstrução a cada redução, quebra descontínua simulada, tolerância de um pixel, leituras finais repetidas, término pelo tamanho solicitado apesar de setter alterar o tamanho efetivo, guardas de altura incluindo NaN, autoScale desabilitado, exceção de reconstrução após mutação e wrap32 no decremento. Medições são fixtures explícitas; não são métricas obtidas do jogo.

Evidência: `build/research/text-time/M19198.asm`, offsets/metadados do APK inventariado; VAs no relatório descontam a base Ghidra0x100000. Fonte concreta e flags iniciais estão em101, dependência de métricas em96–100. Nenhum APK, asset ou decompilação incluído no commit.

Confiança **confirmed** para o fluxo estático examinado; implementação testada contra contratos locais, ainda não contra o original. Linux, fontes reais, parser e renderer permanecem pendentes. Próximo: fechar correção inicial de tamanho e localização, então conectar formação de blocos, ajuste e revelação sem aproximações de métricas.
