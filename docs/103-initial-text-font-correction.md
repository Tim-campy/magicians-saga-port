# Correção inicial do tamanho da fonte

Data: 2026-10-25. Capítulo 4/M3. Complementa102; contrato estático, não comparação visual.

## Ordem e aritmética

TextLayout.SetText19198, trecho0x1bbd298..0x1bbd300, executa a correção somente quando adjustFontSize_+0x86 é false e multiLine_+0x84 é true. A guarda não depende de autoScale_+0x78. Isso importa para falas japonesas, cujo construtor desabilita autoScale sem desabilitar multiline.

O trecho lê o tamanho de Font+0x1c e o fator estático Font.requestFontScale_+0x44. Calcula, nesta ordem:

1. Multiplicação do tamanho por100 com wrap32.
2. Divisão assinada por requestFontScale_, truncando em direção a zero.
3. Multiplicação do resultado por100 com wrap32.
4. Outra divisão assinada pelo mesmo fator capturado.
5. Font.SetSize(resultado,false).
6. Somente após retorno normal, marca adjustFontSize_=true.

Não substituir por uma divisão única nem por aritmética real. Para tamanho10/fator150 o pedido é4. Para tamanho-10/fator150 é-4. Os SDIV não possuem guarda de exceção nesse trecho: divisor zero produz zero; INT_MIN/-1 conserva INT_MIN, segundo a semântica ARM adotada também nos contratos anteriores. O port evita divisão/overflow indefinidos em C++.

O reset de revelação de SetText precede esse trecho, e a primeira construção dos blocos vem depois. O módulo não altera essa ordem por conta própria: o chamador deverá compor os passos.

## Origem e aplicação do fator

Font.SetScale17804 (0x1b7ac34) escreve seu inteiro diretamente em requestFontScale_ estático+0x44, sem clamp observado. O construtor estático de Font inicializa esse campo com100, conforme exportação M17820 do ciclo99. O construtor de instância copia esse valor para fontScale_+0x44. São campos distintos com o mesmo offset em áreas diferentes.

UpdateFontScale17806 (0x1b7a8ac) compara a escala da instância com a solicitada. Se diferentes, copia a solicitada e chama SetSize com o valor original em+0x10 e usePreset=true. _setSize17770 (exportado em99) aplica a escala da instância com multiplicação32/divisão100. Logo, o inteiro calculado por prepare_text_font_size é o **pedido ao setter**, não necessariamente o tamanho final desenhado.

Varredura B/BL imediato nos segmentos executáveis não encontrou chamada direta a SetScale. Essa busca não cobre código inline, escritas diretas, delegates ou reflexão; não prova que o fator permaneça100 durante a partida. O fator permanece entrada explícita do port.

## Implementação e validação

Adicionado prepare_text_font_size ao módulo text_fit, com flag font_size_adjusted em TextFitState. Usa o host existente para ler tamanho e chamar o setter. Nenhuma reconstrução de blocos ocorre nessa função; o chamador a executa antes da primeira reconstrução. Exceções do host propagam; o flag não é marcado após falha do setter.

`tools\build-desktop.cmd`, Windows/MSVC14.51: **19/19 CTests passaram**, log `build/research/text-font-preparation-build.log`. O teste text_fit_contract agora cobre repetição impedida pelo flag, independência de autoScale, exclusão da linha única, divisões sucessivas positivas/negativas, divisor zero, overflow da multiplicação e falha do setter. Os testes de ajuste/reconstrução de102 continuam passando.

Evidência: `build/research/text-time/M19198.asm`; `font-scale-correction/` (SetScale, SetOffset, UpdateFontScale, _adjustFontSize e GetLocalizeScale exportados); `font-scale-callers.json`; `font-selection-default/M17770.asm` e M17820. Endereços deste relatório são VAs do ELF, sem base Ghidra0x100000. Exportar os métodos adicionais não significa que todos os ramos de localização foram fechados.

Confiança **confirmed** para recorte/ordem e escritores descritos; valor efetivo do fator e métricas permanecem **unknown**. Sem execução Android, fontes aproximadas ou conteúdo proprietário versionado. Próximo: integrar a localização de fonte à formação dos blocos, depois compor preparação, construção, ajuste e revelação.
