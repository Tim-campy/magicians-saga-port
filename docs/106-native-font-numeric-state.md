# Estado numérico nativo da fonte

Data: 2026-10-25. Capítulo 4/M3. Complementa os relatórios 99, 103–105.

## Contrato implementado

`src/core/gameplay/text_font` mantém separadamente pedido de tamanho, tamanho efetivo, uso de preset, tamanho ajustado, escala float e escala inteira da fonte. Não cria recursos gráficos nem presume valores de configuração ou conteúdo da tabela de presets.

| Operação | Evidência ELF ARM64 | Comportamento preservado |
| --- | --- | --- |
| `set_text_font_size` | `_setSize17770`, 0x1b78f84 | Stores de tamanho/pedido/flag antes da consulta à tabela; preset quando índice válido; produto com wrap32 e divisão assinada por 100. |
| `update_text_font_scale` | `UpdateFontScale17806`, 0x1b7a8ac | Sem ação se escala igual; caso contrário grava escala e reaplica pedido com presets habilitados. |
| `adjust_text_font_size` | `_adjustFontSize17796`, 0x1b7a858..0x1b7a89c | Recorte posterior à atualização de escala: divide argumento por 100, multiplica/divide tamanho, grava escala, soma viés e converte o tamanho ajustado. |

As duas primeiras operações representam o caminho síncrono, com inicialização de classe já concluída. A terceira deve ser chamada **depois** de UpdateFontScale; ela não reproduz o dispatcher Java/Unity. Não se declara equivalência do agendamento entre threads.

## Tabela e falhas parciais

A releitura de `_setSize` confirmou um detalhe além do resumo anterior: a tabela é carregada e testada contra null em 0x1b78ff0..0x1b78ff8, antes da decisão sobre `usePreset` em 0x1b79008. Assim, tabela ausente falha mesmo com presets desativados, após stores em +0x1c, +0x10 e +0x20. O campo +0x2c permanece com seu valor anterior nessa falha.

Pedido negativo com presets ativos passa pela comparação assinada de comprimento, mas falha no limite unsigned em 0x1b79030. Pedido maior ou igual ao comprimento é interpretado como tamanho literal. Os testes distinguem esses caminhos. A representação nativa usa vector e exceções C++ para sinalizar as falhas; não reconstrói objetos de exceção gerenciados. Arrays válidos da origem têm comprimento int32; vetores maiores estão fora do contrato.

Uma mudança de escala conserva a nova escala mesmo se o setter falha. Não há rollback genérico. A tabela de presets efetiva e seus produtores ainda precisam ser ligados ao boot.

## Precisão

Literal em 0x62798c reconferido como bits `0x3c23d70a`. Intermediários binary32 explícitos e opções de compilação estritas impedem cancelar a multiplicação/divisão ou fundir operações. FCVTZS é modelado sem casts C++ fora do intervalo: NaN vira zero, valores finitos fora da faixa saturam; infinito positivo recebe a substituição explícita por INT_MIN observada no original.

Zero, inclusive negativo, não equivale a ausência de ajuste: a sequência pode produzir NaN. `size` e `requested_size` não são substituídos pelo resultado em `adjusted_size`.

## Validação e limites

`tools/build-desktop.cmd`, Windows/MSVC 14.51: **20/20 testes CTest passaram**, sem warnings encontrados no log `build/research/text-font-state-build.log`. Fixtures cobrem presets/literais, limites negativos, null mesmo sem preset, mudança de escala, efeitos parciais em exceção, wrap32, escala zero/negativa zero, infinito, NaN e limites de conversão.

Evidência local: `font-selection-default/M17770.asm`, `font-scale-correction/F1b7a8ac.asm` e `F1b7a844.asm`, sob `build/research/`. ELF SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Endereços descontam a base Ghidra 0x100000.

Confiança **confirmed** para os recortes estáticos e resultados dos testes nativos. Não há comparação dinâmica com APK, validação Linux ou integração ao parser completo. Próximo: resolver tabela de presets e seu ciclo de configuração, portar localização com suas fronteiras de cleanup e compor preparação/ajuste com a geração de blocos.
