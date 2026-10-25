# Temporização do texto e velocidade por idioma

Data: 2026-10-25. Capítulo4/M3; pesquisa estática, continuação de90.

## Evidência

Exportados métodos TextLayout Init19196, SetText19197/19198, Reset19209, SetSpeed19210, JSystem.CurrentTimeMillis13464 e IApplication.get_TotalFrames18187. Evidências locais em `build/research/text-time/`, `text-progress/` e `text-speed-language/`; seleções TSV adjacentes. Metadados/offsets correspondem ao ELF ARM64 e APK já inventariados. VAs abaixo não incluem a base Ghidra0x100000.

Campos de TextLayout/type2272 resolvidos por metadados e tabela de offsets:

| Offset | Campo |
| --- | --- |
| 0x34 | length_ |
| 0x40 | pos_ |
| 0x44 | speed_ |
| 0x48 | beginTime_ |
| 0x50 | timeOffset_ |
| 0x58 | lastAppFrame_ |

## Velocidade configurada

SetSpeed19210 (0x1bc3568) grava o argumento e chama Language.IsShortWords14061 (0x1b2f410). Se false, substitui por trunc32(wrap32(speed×3)/2); se true, preserva. A multiplicação precede a divisão e pode transbordar32; não usar fator float1.5.

IsShortWords retorna true se Language.Japanese13990 retornar true. Caso contrário, consulta Language.langPackScale_ (campo7788, offset estático0xb0), retornando true se diferente de-1 e >=100. O valor efetivo da configuração do boot não foi determinado aqui.

Consequência: o1500 solicitado em MakeTalkForm pode resultar em1500 ou2250. Não declarar1500 uma constante efetiva universal. Init19196 usa um literal de64 bits em0x626ca0 que inicializa pos=0 e speed=1000.

## Relógio e passagens repetidas

CurrentTimeMillis13464 (0x1be7918) chama DateTime.UtcNow1560, subtração de DateTime1536 com referência estática e TimeSpan.TotalMilliseconds2546, convertendo para int64. Não é evidência de relógio monotônico; a origem exata da referência estática ainda precisa ser conferida.

get_TotalFrames18187 (0x1b7ddc4) retorna o int32 do objeto da aplicação+0x58. TextLayout.Frame19208:

- Se beginTime==0, lê o relógio e grava now-50.
- Compara lastAppFrame ao TotalFrames; se iguais, incrementa timeOffset em50 com largura32.
- Consulta TotalFrames novamente e armazena o valor estendido com sinal em lastAppFrame.
- Lê o relógio novamente, combina now + timeOffset - beginTime e multiplica por speed com largura64.
- O bloco0x1bc3468–0x1bc34ac aplica divisão signed por50 via multiplicação alta/shift, seleciona zero para produto<=-50, limita superiormente pelo produto32 length×1000 e grava os32 bits baixos em pos.

Essas operações exigem preservar larguras, comparações e ordem das consultas. Para valores usuais sem overflow, representam progressão temporal em unidades de milésimos de comprimento. O tratamento exato dos extremos deve ser testado contra a sequência de instruções antes de implementar.

## Reset, mudança de texto e Flush

Reset19209 (0x1bc3554) zera pos, beginTime, lastAppFrame e timeOffset. O caminho de reconstrução de SetText19198 em0x1bbd28c zera pos, beginTime e timeOffset, mas não escreve lastAppFrame nesse trecho. Não substituir indiscriminadamente essa sequência por Reset.

Flush19219 grava wrap32(length×1000) em pos e recalcula beginTime usando relógio e speed. Além disso, Frame atualiza contadores internos dos blocos de texto e um contador de desenho condicional; apenas reconstruir pos ainda não reproduziria TextLayout inteiro.

## Comprimento não é simplesmente o tamanho da string

SetText19198 em0x1bbd420 zera length e percorre textBlocks. Os flags dos blocos+0x44/45 podem fazer um bloco contribuir1; caso contrário há uma chamada sobre o texto do bloco+0x28. Portanto não substituir length por text.size() no modelo preparado em89. Os nomes dos flags, o helper de comprimento e o parser de blocos permanecem próximos alvos.

## Confiança e próximo passo

**confirmed:** nomes/offsets, condição de velocidade, dependência de relógio/TotalFrames e diferença das escritas de Reset/SetText. **unknown:** configuração efetiva de idioma, referência temporal estática, contagem completa de blocos e fidelidade visual.

A sequência signed-high/add/shift da divisão por50 foi comparada à divisão truncada em10.014 valores de fronteira/aleatórios (seed91), todos aprovados; registro local em build/research/text-time/division-check.txt. Isso verifica a simplificação aritmética, não execução do APK nem o método inteiro.

Sem alteração C++; nenhum teste de produto repetido. Próximo: resolver cálculo de length e fronteira de configuração antes de implementar o relógio portátil. Não há bloqueio dinâmico: as dependências ainda podem ser investigadas nos artefatos locais.
