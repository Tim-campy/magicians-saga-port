# Contagem dos blocos e estado temporal nativo do texto

Data: 2026-10-25. Capítulo4/M3. Continuação dos contratos estáticos de90/91; sem validação dinâmica.

## Nova evidência de comprimento

TextBlock/type2268 tem text_+0x28, isButton_+0x44 e isPictograph_+0x45. Metadados: campos11003–11021, tabela de offsets do ELF identificado. SetText19198 em0x1bbd420–0x1bbd48c soma1 se qualquer desses flags estiver ativo; caso contrário soma StringEx.Length13492 (VA0x1be83d4) do texto do bloco.

Length lê diretamente o comprimento int32 da string gerenciada+0x10; null segue a exceção. Portanto conta unidades UTF-16, inclusive duas para um par substituto, sem contar bytes ou grafemas. Um bloco com ambos os flags conta somente1 e não acessa seu texto. A acumulação usa largura32.

Evidências locais: `build/research/text-block-length/M13492.asm`, `M19200.asm`, seleção TSV adjacente e `text-time/M19198.asm`. M19200 confirma escritas nos flags; o parser completo e suas tags ainda não foram reconstruídos.

## Implementação

Novo módulo C++17 `src/core/gameplay/text_reveal.*`:

- Contagem de uma coleção já interpretada de blocos. Não interpreta marcações; rejeita bloco nulo ou texto ordinário nulo. Flags gráficos permitem texto nulo.
- Transformação aritmética de SetSpeed, recebendo explicitamente o resultado de IsShortWords.
- Estado com length, position, speed, beginTime, timeOffset e lastFrame.
- Reset completo e reinício temporal do caminho de reconstrução de SetText separados; o segundo preserva lastFrame.
- Trecho temporal de Frame, Flush e IsEnd, usando interface de relógio/TotalFrames.

O relógio é fornecido pelo host. Não foi substituído por relógio monotônico, timestep fixo ou número de caracteres por frame. Consultas preservadas: se beginTime==0, relógio inicial; depois duas leituras de TotalFrames e leitura final do relógio. Overflow é reproduzido com operações unsigned e preservação dos bits, evitando overflow signed indefinido em C++.

Frame usa produto64, divisão por50 truncada, seleção de zero quando produto<=-50 e limite superior calculado em32 bits. Flush grava o limite, consulta o relógio e recalcula beginTime com multiplicação32→multiplicação64→SDIV por speed→divisão por-20. O SDIV do artefato não tem guarda de divisor zero: quociente0 é preservado. Não é uma afirmação sobre o comportamento geral de divisões C#.

## Caso de arredondamento

Para length1, speed1500 e now2000, Flush grava position1000 e beginTime1967. Uma atualização seguinte com now2000, sem nova compensação, calcula position990. Não impor um flag de conclusão permanente. Esse resultado é derivado das instruções e coberto por teste; não foi observado executando o jogo.

## Validação

Comando: `tools\build-desktop.cmd`. **Windows/CTest17/17 aprovado**, sem warnings/errors no log `build/research/text-reveal-build.log`.

Teste novo cobre UTF-16, flags gráficos, null, velocidade/overflow, ordem das consultas, chamadas repetidas no mesmo frame, tempo retrocedendo, diferenças de reset, limite e overflow32/64, Flush, divisor zero e sentinel-1. Os dados são fixtures próprias, não assets do jogo.

## Limites e próximo passo

Confiança **confirmed** para os contratos estáticos citados; implementação testada localmente, sem equivalência dinâmica demonstrada. A função temporal não substitui Frame inteiro: blink dos blocos, contador de desenho, parser, fontes, layout e renderização permanecem fora dela. O comprimento vem de blocos já interpretados, nunca de uma estimativa sobre o texto preparado.

A preparação de formulários de89 ainda não instancia um TextLayout completo. Próximo: reconstruir a geração dos blocos necessários às falas reais, configuração de idioma e composição com avanço de páginas. Não há bloqueio por ausência de Android enquanto essas investigações estáticas avançam.
