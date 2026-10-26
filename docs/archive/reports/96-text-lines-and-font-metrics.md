# Formação das linhas e dependência de métricas da fonte

Data: 2026-10-25. Capítulo4/M3; investigação estática, sem comparação visual.

## Pergunta e evidência

Como os textos preparados e parâmetros portados chegam aos blocos consumidos por text_reveal? Inspecionados _makeTextBlocks19200, TextBlock..ctor19271, Font.StringWidthF17782/17783 e Font._stringWidthF17784. Endereços abaixo são VAs do ELF ARM64 inventariado; exports Ghidra têm base adicional0x100000.

Evidência local: build/research/text-block-length/M19200.asm, text-metrics/, font-width/Width.asm, font-metrics-core/ e font-texture-size/GetFontTextureSize.asm, com seleções TSV adjacentes.

## Quebra e organização das linhas

Em0x1bbde50–0x1bbdec4, LF (U+000A) seleciona a quebra explicitamente. Para outros caracteres, compara largura acumulada+incremento com TextLayout.width_+0x38; width=-1, flags de processamento e multiLine_+0x84 mudam o caminho. Com format_+0x6c ativo, U+3001/U+3002 têm tratamento especial no trecho de estouro. Não generalizar isso como quebra a cada N caracteres.

O incremento pode ser calculado medindo o texto concatenado ao bloco e subtraindo sua largura anterior (0x1bbdee4–0x1bbdf34). Logo a soma de larguras isoladas dos caracteres não reproduz necessariamente a medida original.

Há caminho adicional de antecipação de palavra/trecho (0x1bbdf40–0x1bbe318), com parada em espaço/LF e inspeção de tags. Em0x1bbe338 chama Language.IsJapaneseStyleTextLayout13996; a resposta afeta o espaço na quebra. Seleção de idioma e todos os guardas desse caminho ainda não estão fechados.

Ao fechar uma linha, em0x1bbe508–0x1bbe668:

- Percorre os blocos da linha e grava x a partir do deslocamento de alinhamento, somando suas larguras.
- Grava y a partir do acumulador vertical mais metade inteira da diferença entre a altura máxima da linha e a altura da fonte do bloco.
- Zera largura acumulada e avança y por altura máxima+lineSpace_.
- Há um ramo adicional ligado a ff/autoScale que não deve ser descartado ao reconstruir paginação.

A aplicação geométrica confirma os bits de po descritos em93: horizontal2 usa centralização e4 alinha à direita; vertical0x20 usa meio e0x40 parte inferior. Dimensão-1 possui fórmulas próprias. Ainda falta a composição completa com escalas, margens e limites.

## Construção do bloco

TextBlock..ctor19271 (0x1bc09e8) recebe parent/font/color/x/y/blink, cria uma cópia do objeto Font e inicializa o texto vazio. start_ e length_ recebem-1, borderColor recebe-1. Não é apenas uma vista de caracteres sobre a string original.

Os trechos que atribuem text_ por substring e atualizam width dependem das decisões anteriores de medição e quebra; o modelo de contagem de92 recebe esses blocos já formados, não os produz.

## Cadeia de medição

| Método | VA | Papel confirmado |
| --- | --- | --- |
| Font.StringWidthF17782 | 0x1b798ec | Encaminha ao overload com argumento adicional nulo. |
| Font.StringWidthF17783 | 0x1b79778 | Consulta IApplication.CheckUiThread18256; chama medição direta ou despacha para UI. |
| Font._stringWidthF17784 | 0x1b798f8 | Configura fonte, escala e conteúdo antes da medição. |
| Font.GetGUIStyle17819 | 0x1b7a310 | Fornece GUIStyle usado pela rota Unity. |
| GUIStyle.CalcSize58221 | 0x23e9e28 | Mede GUIContent quando useFontTexture_ é falso. |
| KairoPlugin.GetFontTextureSize14837 | 0x1c2b724 | Rota alternativa quando useFontTexture_ é verdadeiro. |

Font.useFontTexture_ é o campo de instância+0x48. Há outros campos estáticos com offsets coincidentes; não confundir com requestFontOffset_. Font.guiStyleId_+0x24 igual-1 ou texto nulo produzem retorno0 na entrada dessa medição.

No caminho padrão são configurados GUIStyle.fontSize/fontStyle e GUIContent.text. Há cache de substituição de espaços por pontos para uma condição examinada pelo método; a condição completa ainda não foi fechada. O resultado medido recebe escala por100 e divisão pelo fator obtido da aplicação.

A rota de textura calcula um tamanho escalado e chama GetFontTextureSize; o wrapper captura texto/tamanho num objeto e despacha um delegate. Esse delegate e o backend final ainda precisam ser resolvidos. Não atribuir ao Android nem a uma biblioteca desktop sem essa evidência.

## Consequências para o port

As fontes extraídas no relatório14 continuam úteis, mas sua presença não comprova métricas equivalentes às duas rotas originais. Não integrar um tamanho constante por caractere nem declarar que uma fonte escolhida no desktop terá as mesmas quebras.

O próximo contrato de layout precisa de medição de trechos, cópia/configuração da fonte, escala e seleção de backend comprovadas. O núcleo continuará independente de Unity/Android; a substituição desktop terá de preservar resultados e ser validada quando houver referência dinâmica.

Confiança **confirmed** para chamadas, offsets e operações citadas; **unknown** para backend efetivo por idioma, fidelidade de métricas e equivalência visual. Nenhuma implementação aproximada adicionada, nenhum CTest repetido: última validação C++ permanece18/18 em95.

Próximo: resolver Font.useFontTexture_ e o delegate de GetFontTextureSize, além da política de fonte/escala usada pelo boot. Esses passos ainda permitem avanço estático sem Android.
