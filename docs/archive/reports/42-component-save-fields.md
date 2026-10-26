# Campos persistidos de animação, direção e SEB

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Continuação estática de41, sem saves reais ou execução Android. Endereços são VAs ELF; exports somam0x100000.

## Sequência dos campos

Atualização posterior: relatório43 fecha ordem big-endian/truncamento16 e implementa os payloads fixos de Animation e Direction. As pendências de conversão abaixo registram o estado desta rodada anterior; stream completo e Seb continuam separados.

| Componente | Serialize / Deserialize | Ordem de gravação e leitura |
|---|---|---|
| Animation | 37261:0x113e3a8 / 37262:0x113e424 | +0x10 rate por WriteByte/ReadByte; +0x14 retorno de cut-in por WriteShort/ReadShort |
| Direction | 37296:0x113ecf0 / 37297:0x113ed6c | +0x10 direção por byte; +0x14 locked por booleano |
| Seb | 37416:0x11417a8 / 37417:0x1141848 | +0x10 recurso por byte; +0x14 clip por short; +0x18 frame por byte; +0x20 layers por array compacto com parâmetro de comprimento1 |

Os nomes dos campos funcionais vêm do mapeamento de produtores/transições nos relatórios30/31; os offsets e chamadas de persistência foram conferidos novamente. Leituras byte destes componentes aplicam **SXT[B]** antes de armazenar em inteiro32; leituras short aplicam **SXT[H]**. Portanto byte0xff carrega -1, e byte0x80 carrega -128. Não usar uint8 como valor final de frame/rate/recurso/direção.

WriteByte14470 (`0x1c0de6c`) aplica SXT[B] ao argumento e encaminha ao slot7 do stream. WriteShort14472 (`0x1c0deac`) chama conversor em0x1bf34a0 com flag1 e grava os bytes do resultado sequencialmente pelo mesmo slot. A ordem dos bytes e a política do conversor para valores fora da faixa ainda devem ser examinadas antes de implementar um encoder. Não declarar round trip de qualquer inteiro32: a leitura comprovadamente reduz ao valor com sinal de8/16 bits.

Cada Deserialize grava um campo assim que a respectiva leitura termina. Não há objeto temporário completo nem rollback nesses corpos. Uma falha posterior pode deixar campos anteriores já alterados. Não transformar isso silenciosamente em carregamento transacional de todo o componente.

## Array layers de SEB

Seb usa StreamUtil2.WriteIntArrayAsByteArray36547 (`0x1125098`) e ReadIntArrayFromByteArray36548 (`0x1125338`), ambos com argumento de comprimento1.

Nesse caminho de gravação:

- Array null escreve comprimento-1 como byte e não escreve elementos.
- Array não nulo exige comprimento **menor que127**;127 já segue a saída excepcional.
- Comprimento0 é escrito e não há elementos.
- Cada inteiro do array é encaminhado a WriteByte, em ordem. O parâmetro1 controla a largura do comprimento, não uma largura variável dos elementos.

Na leitura, o comprimento byte sofre extensão de sinal. -1 retorna null. Outros valores são usados para alocar array de inteiros; não há normalização de outros negativos para null. Cada elemento ReadByte sofre extensão de sinal antes de ser guardado em inteiro32. Assim null, array vazio e array[-1] são estados distintos do formato; o último usa um elemento0xff, não comprimento0xff.

As variantes de comprimento2/4/8 também aparecem nos helpers, mas não são usadas por Seb neste caminho e não foram portadas.

## Booleano e comportamento do leitor

Atualização posterior: relatório47 resolve o stream usado por LoadGame. MemoryStream.Read retorna0 no EOF, mas InputStream.Read o converte para-1 antes de StreamUtil. Portanto a possibilidade genérica de repetição sem progresso descrita abaixo não se aplica ao EOF normal dessa cadeia concreta.

ReadBoolean14445 (`0x1c0c6e8`) lê um byte e retorna **byte==1**. Valores2 ou255 são false; não usar comparação diferente de zero. WriteBoolean14478 (`0x1c0e0dc`) encaminha argumento&1 ao stream. Essa máscara não transforma qualquer inteiro não nulo em1.

ReadByte14434 (`0x1c0c344`) pede1 byte a StreamUtil.Read14430 (`0x1c0be50`). ReadShort14436 (`0x1c0c42c`) pede2 e encaminha ao conversor0x1bf36a8 com flag1. ReadInt14439 (`0x1c03458`) pede4 e encaminha a EncodeUtil.ByteAryToInt13773 (`0x1bf0bc4`) com flag1. Este último chama helper0x1bf2f90 antes de conversão quando o flag está ligado. Endianness não foi concluído somente pelo nome do flag.

StreamUtil.Read tenta acumular a quantidade pedida em um stream temporário. Lê para buffer com offset0 e quantidade restante; quando recebe tamanho positivo, escreve essa porção no acumulador. Retorno negativo encerra a tentativa e, se insuficiente, segue para erro. **Retorno zero com quantidade ainda faltante não avança nem termina: o loop chama Read novamente.** Se o stream continuar retornando zero, o corpo analisado não termina. Não foi executado um teste que force esse caso e não se afirma que ele aconteça em saves normais.

Quantidade-1 retorna null imediatamente. Quantidade0 não entra no loop e retorna o conteúdo vazio do acumulador. Não foi implementado um leitor que silenciosamente troque o caso zero persistente por exceção e depois seja declarado idêntico ao original. Uma futura ferramenta de inspeção offline pode precisar de limite explícito e documentado; isso deve permanecer distinto do contrato comportamental do jogo.

## Validação e próximos passos

Exports locais `build/research/component-persistence.tsv` (11 FDEs) e `component-stream-followup.tsv` (7 FDEs), todos com cobertura sintática integral em coverage.tsv. Chamadas, extensão de sinal, ordem de stores, guardas de comprimento e comparações de booleano foram lidas no assembly. Nenhuma fixture de save real foi criada e nenhum C++ foi alterado; sem novo build alegado.

Próximo: fechar EncodeUtil/endianness, stream concreto e callbacks extras de componentes; então implementar codecs de payloads com fixtures sintéticas ancoradas nos bytes e limites comprovados. O formato externo de save, referências e preparação do mundo seguem pendentes. Ainda há investigação estática útil.
