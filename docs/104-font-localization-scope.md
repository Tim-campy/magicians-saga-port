# Localização temporária da fonte na formação dos blocos

Data: 2026-10-25. Capítulo 4/M3. Análise estática, sem métricas ou execução Android.

## Entrada no parser

TextLayout._makeTextBlocks19200 salva a referência original de font_ em0x1bbd824. Se localizeFontEnable_+0x85 é false, chama Font.PushLocalizeSetting(false) em0x1bbd84c antes de perguntar IsNeedLocalizeScale17797 (0x1b79fe0).

Se o predicado retorna true, o parser cria uma cópia da fonte, troca font_+0x18, chama GetLocalizeScale17798 (0x1b7a128) sobre a cópia e passa seu inteiro a **SetSize com usePreset=true**, em0x1bbd8b8. Depois empilha false em0x1bbd8d4, evitando nova aplicação da localização dentro desse escopo. O pedido ao setter não equivale necessariamente ao tamanho final, conforme103.

O método também cria outra cópia para o trabalho dos blocos e chama AdjustFontSize com scaleRatio_+0x7c em0x1bbd974. Essa operação é distinta da redução localizada descrita aqui; não fundir seus fatores por álgebra.

## Predicado IsNeedLocalizeScale

Campos estáticos de Font confirmados: localizeSettingLock_+0x30, localizeSettingStack_+0x38 e localizeSettingCount_+0x40. A permissão é obtida assim:

- Se count>=1, ler o item de índice count-1 da lista. O topo substitui a configuração global.
- Caso contrário, usar Config.LOCALIZE_FONT_SCALE, byte estático+0x50.

Mesmo com permissão, retorna false se localizeSettingLock_ é true ou Language.Japanese retorna true. Por fim, exige size_ de instância+0x1c >5. A guarda de tamanho vem depois das consultas anteriores, ordem que importa para eventual implementação por callbacks.

PushLocalizeSetting reutiliza a lista: quando falta slot, acrescenta um elemento; escreve o bool no índice count e incrementa count. PopLocalizeSetting apenas decrementa count quando count>=1; não remove elementos da lista e não lança underflow nesse caminho. Isso explica por que o armazenamento retém valores além do topo ativo.

## Tamanho localizado

GetLocalizeScale retorna um inteiro de tamanho, apesar do nome. Começa com fator binary32 `0x3f547ae1` (aproximadamente0,83). Se Config.ASIA_FONT_SCALE, byte+0x6d, está ativo, consulta Language.GetLanguageCode e testa prefixos `ko` e `zh` usando String.StartsWith567, segundo argumento1. Se algum corresponde, usa `0x3f7ae148` (aproximadamente0,98). A semântica cultural desse overload não foi reimplementada neste ciclo.

Converte size_ para float, multiplica pelo fator e soma `0x3dcccccd` (aproximadamente0,1), mantendo operações separadas. Usa FCVTZS para inteiro e um tratamento explícito de infinito positivo. Para tamanho original<=9, limita o resultado inferiormente a size_-1 (subtração32). Para tamanhos maiores não aplica esse limite. Os casos extremos de conversão precisam manter a semântica ARM; não usar cast C++ com valor fora do intervalo.

Os valores binários foram lidos respectivamente em0x62782c,0x627900 e0x6278b8 do ELF. Não substituir por fatores decimais de outra precisão durante a reconstrução.

## Saída e restauração

O helper de limpeza0xf875bc, já estudado em95, recebe ponteiros para o bool indicando localização, fonte original e layout:

- Em0xf875ec, desativa LT_DISABLE_TEXTLAYOUT.
- Se o bool de localização está ativo, faz PopLocalizeSetting em0xf87610 e restaura a referência original em font_ em0xf87620.
- Se localizeFontEnable_ está false, faz o Pop correspondente em0xf8764c.
- Em0xf8766c chama Graphics.PopAutoTextLayout; depois verifica limites dos blocos e pode relançar a exceção guardada, conforme95.

Essa sequência fecha o caminho normal e o uso do helper já identificado. Não equivale a afirmar que todas as falhas anteriores à montagem do contexto percorrem o mesmo cleanup. O parser chama tradução/Chanakya e constrói blocos dentro de um escopo maior de tratamento; um RAII genérico cobrindo a função inteira pode mudar o comportamento em falhas de preparação.

## Evidência e limites

Exportações `build/research/font-localize-scope/` (predicado/push/pop), `font-scale-correction/F1b7a128.asm`, `text-block-length/M19200.asm` e `text-parser-cleanup/Ff875bc.asm`. Literais/offsets conferidos no ELF/metadados inventariados. Endereços deste relatório excluem a base Ghidra0x100000.

Confiança **confirmed** para guardas, constantes e sequência descrita; valores efetivos da configuração e métricas **unknown**. Nenhuma mudança C++ neste ciclo, nenhum teste de produto repetido; último build19/19 em103. Próximo: fechar a aplicação de AdjustFontSize e os limites de exceção da preparação antes de integrar esse escopo ao parser nativo.
