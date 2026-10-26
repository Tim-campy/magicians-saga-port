# Divisão nativa dos textos e regra exata de trim

Data: 2026-10-25. Contrato recuperado do ELF ARM64; VAs sem base Ghidra0x100000. Sem execução do APK.

## Regra confirmada

StringUtil.Split14533 encaminha à implementação14535 com flags(1,0,0). Em14535, o primeiro flag é preservado em w21. No trecho0x1c192cc–0x1c193a4, esse flag controla dois laços sobre unidades UTF-16:

- Avança início enquanto a unidade é menor que0x21.
- Recua fim enquanto a unidade é menor ou igual a0x20.
- Emite substring quando o comprimento restante é positivo; caso contrário, emite string vazia.

Logo o trim remove das bordas **U+0000..U+0020**, não todos os caracteres Unicode classificados como espaço. U+00A0 (NBSP), U+3000 (espaço ideográfico) e surrogates permanecem. O interior da string não é alterado.

String.Trim625/TrimWhiteSpaceHelper630 também foram exportados para comparação, mas não são chamados por esse trecho. Não usar a regra de Char.IsWhiteSpace dessas rotinas para implementar o split dos formulários. O tratamento Unicode considerado pendente em85/86 foi esclarecido para este caminho específico.

## Vazio, delimitadores e ordem

O início de14535 retorna array de comprimento zero para entrada null ou string vazia. Para entrada não vazia, preserva segmentos vazios porque o flag de exclusão está desligado. Assim TAB sozinho produz dois segmentos vazios; texto só com caracteres removíveis produz um segmento vazio. Delimitador ao final conserva o último campo vazio.

Nos formulários, separador TAB usa trim. A divisão interna de MakeEvent por `&` usa flags(0,0,0), portanto conserva os espaços dos campos. Nenhuma dessas duas chamadas habilita escapes. Normalização condicional de CRLF só ocorre no ramo cujo separador é LF; não se aplica a estes delimitadores.

## Implementação e verificação

`src/core/gameplay/presentation_text.{hpp,cpp}` fornece `split_form_text` e `split_event_text_fields`, operando em `std::u16string` para preservar as unidades da string original. São etapas posteriores à substituição/tradução, não implementações desses serviços. Não há decoder UTF-8, parser de tipo numérico ou renderer escondido nesse módulo.

Teste `presentation_text_contract` cobre null/vazio, delimitador final, entrada de controles, NUL interior, ausência de escapes e a composição TAB→`&`. Uma varredura de todas as unidades UTF-16 verifica a regra das bordas; TAB é testado separadamente por ser o delimitador. Também verifica que espaços japoneses e surrogates não são removidos.

`tools\build-desktop.cmd`: **14/14 testes CTest passaram em Windows**. Log local: `build/research/presentation-text-build.log`. Nenhum teste Linux, gráfico ou comparação dinâmica com o original foi realizado.

## Configuração ainda pendente

Busca de BL/B imediato ao setter StringUtil.set_SanSanRemove14529 (VA0x1c18d9c) não encontrou callers nos segmentos executáveis examinados. Resultado local: `build/research/sansan-setter-callers.json`. Isso não exclui reflexão, escrita direta no campo ou chamadas indiretas; não resolve sozinho o estado efetivo da propriedade. Não foi imposto valor presumido ao port.

O ciclo anterior foi progresso ao fechar o produtor do catálogo. Neste ciclo, uma regra de texto antes incerta tornou-se código nativo verificado. Próximo: fechar a configuração/caminhos de tradução e a substituição de argumentos para integrar a preparação completa dos formulários, conservando as diferenças entre texto null, vazio e campos vazios.
