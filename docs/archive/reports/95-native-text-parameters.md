# Limpeza externa do parser e parâmetros numéricos nativos

Data: 2026-10-25. Capítulo4/M3. Análise estática e testes locais; sem execução do APK.

## Handler externo: limpeza não significa recuperação

Continuação de94. A chamada decimal do primeiro canal RGB em0x1bbe498 tem landing pad0x1bbf930, que encaminha ao tratamento externo. Em0x1bbfbbc o seletor1 captura o wrapper e grava o objeto de exceção emsp+0x70. Em0x1bbfbd8 chama a rotina nativa0xf875bc com esse endereço.

A rotina0xf875bc não é um descarte trivial: encerra estado temporário de fonte/layout, faz verificações de área e, ao final0xf87710, lê o primeiro campo do contexto. Se não nulo, chama0xfd3d20, que chama0x104f9c4. Esse corpo aloca wrapper e chama __cxa_throw. Os imports foram conferidos em .plt/.rela.plt:0x25fe500 begin_catch,510 end_catch,520 allocate_exception,530 throw.

As rotas0x1bbfc88/0x1bbfcbc garantem a escrita true em Language+0x184 e relançam o objeto/propagam a exceção. Campo7828 é UnicodeToChanakyaEnable. A restauração escreve true, não uma cópia do valor anterior.

Portanto os erros dos canais RGB não são convertidos em resultado0 pelo catch externo. A limpeza também pode lançar erros próprios: não tratar toda saída como retorno normal.

## Efeitos de limpeza identificados

- Font/type2144, offset estático0x4c: LT_DISABLE_TEXTLAYOUT, campo9805. A rotina escreve false.
- Chamadas a Font.PopLocalizeSetting17802 e Graphics.PopAutoTextLayout18049.
- Possível restauração da fonte do TextLayout e verificação geométrica de blocos contra a área.
- Na construção normal, TextLayout.textBlocks_ recebe o novo array em0x1bbf528, antes do preenchimento final. Não há evidência de rollback transacional de todo o objeto em caso de erro.

Não implementada ainda essa limpeza inteira: ela depende de fonte, geometria e estados que o port ainda não possui.

Evidências locais: build/research/text-parser-cleanup/Ff875bc.asm, F1b62c34.asm; text-raise/Raise.asm; text-raise-body/RaiseBody.asm; text-block-length/M19200.asm e text-integer-exceptions.json. Endereços no relatório são VAs ELF, sem base Ghidra.

## Implementação dos parâmetros

Novo módulo text_parameters recebe o conteúdo já extraído entre = e > e o índice original da tag. Preserva:

1. Tentativa decimal; somente falha representando System.Exception é recuperada como0.
2. Tentativa hexadecimal independente; mesma recuperação local.
3. Para co/bo (índices0/10) com hexadecimal0, se houver vírgula, split com trim U+0000..U+0020, parse decimal dos primeiros três campos e combinação por shifts/OR. Campo adicional é ignorado; erros dos componentes propagam.
4. Extração dos três bytes e alpha0xff para o valor de cor.

TextParameterHost exige as conversões decimal/hex; ManagedTextParseError representa a categoria gerenciada que os handlers locais capturam. Exceções C++ não relacionadas não são engolidas. Não foi introduzido um parser numérico aproximado ou uma cultura presumida.

Graphics.GetColorOfRGB17946 (0x1b62c34) limita seus argumentos a0..255 e acrescenta alpha0xff; neste chamador os argumentos já foram extraídos dos bytes. A combinação anterior não limita os componentes RGB: por exemplo1,256,0 resulta em0xFF010000 e1,2,-1 em0xFFFFFFFF. O helper nativo preserva essa ordem.

split_text_color_fields reutiliza o splitter UTF-16 já verificado. Nenhuma nova regra de whitespace ou de campos vazios foi criada.

## Validação

`tools\build-desktop.cmd`: **Windows/CTest18/18 aprovado**; log local build/research/text-parameters-build.log.

Novo teste cobre recuperação local, resultados decimal/hex distintos, cor alfanumérica, espaços, dois/três/quatro componentes, overflow/OR de canais, tag não-colorida, erro no segundo canal e erro nativo não capturável. Hosts numéricos de teste são fixtures explícitas; não validam a implementação real de Int32.Parse.

## Estado e próximo passo

Confiança **confirmed** para os encaminhamentos, escritas, cores e sequência de parâmetros citados. Recuperação local está portada; limpeza completa, parser de marcações, métricas, tradução configurada e renderer continuam pendentes.

Próximo: integrar reconhecimento e aplicação das tags comprovadas à geração de blocos, preservando fronteiras de tradução/fonte e evitando tratar o texto preparado como layout pronto. Ainda há trabalho estático útil; Android não é necessário para este avanço.
