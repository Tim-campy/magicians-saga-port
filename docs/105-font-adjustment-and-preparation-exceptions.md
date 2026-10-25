# Ajuste numérico de fonte e exceções da preparação

Data: 2026-10-25. Capítulo 4/M3. Complementa104 com a sequência aritmética e fronteiras de limpeza.

## AdjustFontSize

Font.AdjustFontSize17795 (0x1b7a6cc) consulta CheckUiThread. No caminho direto, chama UpdateFontScale17806 antes de ler o tamanho em+0x1c. No caminho delegado, grava o argumento float em+0xa8 e agenda o delegate17829 (0x1b7c460), que lê esse campo, chama UpdateFontScale e repete a sequência. _adjustFontSize17796 (0x1b7a844) possui a mesma aritmética.

Sequência após UpdateFontScale, com operações binary32 separadas:

1. q = argumento /100.0f.
2. Converter o tamanho inteiro atualizado para float.
3. Multiplicar por q.
4. Dividir o produto por q.
5. Guardar q em scale_+0x40.
6. Somar o literal binary32 de0x62798c (`0x3c23d70a`, aproximadamente0,01).
7. Converter para inteiro por FCVTZS; se o valor comparado é exatamente infinito positivo, substituir por INT_MIN.
8. Guardar o inteiro no campo+0x2c.

Não cancelar multiplicação/divisão por q. O código pode receber zero, infinitos ou NaN; para tamanho finito e argumento zero, a divisão0/0 produz NaN e FCVTZS produz0 segundo a semântica ARM, não o tamanho original. Isso é uma consequência da sequência, não uma medição do aplicativo. Mesmo para valores finitos, arredondamentos intermediários devem permanecer.

O argumento passado por _makeTextBlocks em0x1bbd974 vem de TextLayout.scaleRatio_+0x7c. A função não grava o inteiro de volta em size_+0x1c nesse recorte: size_, scale_ e o campo+0x2c são estados distintos. UpdateFontScale pode ter alterado size_ antes dessa aritmética, conforme103.

O caminho delegado usa um campo mutável da própria instância, não um argumento imutável capturado por valor. A política de despacho/sincronização continua responsabilidade da camada correspondente; não inferir execução assíncrona ou corrida apenas pela existência do delegate.

## Fronteira de limpeza no parser

LSDA0x935e34 do FDE _makeTextBlocks19200, extraída em94, foi cruzada com os BLs e seus landing pads:

| Chamada no parser (VA ELF) | Operação | Landing pad | Destino comum |
| --- | --- | --- | --- |
| 0x1bbd84c | PushLocalizeSetting(false) inicial | 0x1bbfa30 | 0x1bbfa30 |
| 0x1bbd860 | IsNeedLocalizeScale | 0x1bbf9dc | 0x1bbfa30 |
| 0x1bbd88c | construtor da cópia localizada | 0x1bbf9d8 | 0x1bbfa30 |
| 0x1bbd8a4 | GetLocalizeScale | 0x1bbfa08 | 0x1bbfa30 |
| 0x1bbd8b8 | SetSize localizado | 0x1bbfa08 | 0x1bbfa30 |
| 0x1bbd8d4 | PushLocalizeSetting(false) após ajuste | 0x1bbfa08 | 0x1bbfa30 |
| 0x1bbd958 | construtor da fonte de trabalho | 0x1bbfa28 | 0x1bbfb9c |
| 0x1bbd974 | AdjustFontSize | 0x1bbfa24 | 0x1bbfb9c |
| 0x1bbd9b0 | TranslateText | 0x1bbf9d0 | 0x1bbfb9c |

Todos esses registros têm action_index3, mas o índice sozinho não identifica o caminho de limpeza. Em0x1bbfa30, salva exceção/seletor e salta diretamente para0x1bbfc88, **sem chamar o helper interno0xf875bc nesse trajeto**. Em0x1bbfb9c, segue para0x1bbfbbc: o seletor1 captura o wrapper e entrega o objeto ao helper; outros seletores seguem a0x1bbfc80 e também o chamam. Os detalhes do relançamento do helper estão em95.

O contexto do helper é montado em0x1bbd8fc..0x1bbd914, entre os dois grupos. A restauração externa de UnicodeToChanakyaEnable=true e o relançamento já foram mapeados em95. Não existe base para restaurar genericamente fonte/pilha em toda falha de preparação: o trajeto anterior ao contexto pula essa limpeza, inclusive após stores já executados. Não se afirma que callers externos nunca restaurem estado; apenas que esse caminho local não o faz.

## Consequência para a reconstrução

Separar preparação da localização e corpo protegido. Uma implementação deve preservar mutações anteriores a falhas, ativar o cleanup interno somente na fronteira comprovada e manter a finalização externa separada. Um único guard de escopo que restaura tudo desde a entrada mudaria o comportamento recuperado.

## Evidências e validação

Exportações `build/research/font-adjust-wrapper/` (wrapper/delegate), `font-scale-correction/F1b7a844.asm`, `text-block-length/M19200.asm` e `text-integer-exceptions.json`. Seleção dos nove registros em `font-adjust-wrapper/preparation-exceptions.json`, com hash ELF e LSDA. Branches dos landing pads conferidos no disassembly; endereços aqui descontam base Ghidra0x100000.

Confiança **confirmed** para sequência e fronteiras locais; valores efetivos e resultado visual **unknown**. Sem alteração C++ ou repetição de testes de produto; último build19/19 permanece103. Próximo: implementar os estados e operações de fonte necessários ao parser, preservando essa divisão de escopos, e conectar os contratos de preparação/ajuste já existentes.
