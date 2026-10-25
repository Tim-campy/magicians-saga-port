# Ordem dos bytes e codecs de payload fixo

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Continuação de42, somente análise estática e testes do código portátil. VAs ELF abaixo; Ghidra adiciona0x100000.

## Endianness confirmado para esta cópia

EncodeUtil.ShortToByteAry13765 (`0x1bf34a0`) chama BitConverter.GetBytes1040 (`0x1ddd674`), que aloca2 bytes e executa STRH do valor recebido no início do array. No ELF AArch64 little-endian, isso grava os16 bits baixos em ordem little-endian. IntToByteAry13761 (`0x1bf3338`) chama GetBytes1041 (`0x1ddd6d8`), que aloca4 e executa STR, analogamente.

Quando o argumento de conversão está ligado, ambos encaminham a ConvertEndian13777 (`0x1bf2f90`) com tamanho2/4. Esse corpo troca os bytes simétricos do array. Os wrappers WriteShort/WriteInt de42 passam flag1: portanto a sequência emitida é **big-endian**, independentemente do nome escolhido para esse flag. GetBytes1040 usa STRH sem checagem de faixa, comprovando truncamento aos16 bits baixos.

ByteAryToShort13775 (`0x1bf36a8`) inverte2 bytes quando flag1 e chama BitConverter.ToInt161049 (`0x1ddda24`), que lê LDRH. O componente então aplica SXTH, como visto em42. O fluxo de leitura é inverso ao de gravação. A conclusão é específica ao artefato ARM64 examinado; não foi deduzida por convenção de formato ou por fontes de outra versão.

ConvertEndian pode alocar outro array quando comprimento recebido difere do tamanho solicitado. Os codecs abaixo não reproduzem esse caminho genérico: recebem exatamente os bytes do payload, sem wrapper de stream ou arrays de comprimento arbitrário.

## Implementação

Atualização posterior: relatório44 acrescenta payload variável Seb/layers. A limitação mencionada nesta rodada foi resolvida para buffers completos; stream incremental e save completo continuam pendentes.

`src/core/animation_payload.hpp/.cpp` adiciona codecs sem dependências de plataforma:

| Payload | Bytes | Interpretação |
|---|---:|---|
| AnimationPlayback | 3 | rate8 com sinal; return_clip16 big-endian com sinal |
| DirectionState | 2 | direction8 com sinal; locked verdadeiro somente para byte1 |

Encode reduz os campos inteiros aos8/16 bits baixos, conforme serializadores recuperados. Decode faz extensão de sinal explicitamente, sem cast unsigned→signed fora de faixa dependente de implementação C++. O booleano C++ de entrada codifica0/1; decode aceita todos os256 padrões e só1 produz true.

Exemplos de bytes derivados das instruções: Animation(1,-1)→`01 ff ff`; Animation(-128,0x1234)→`80 12 34`; Direction(-1,true)→`ff 01`. Animation(256,0x12345678) reduz a `00 56 78`; isso não equivale a preservar todo inteiro32 em round trip.

A API usa std::array de tamanho exato. Não aceita stream nem pretende reproduzir leitura parcial, EOF, mutações intermediárias do objeto original ou o loop sem progresso documentado em42. É um codec de payload completo, que deverá ser integrado a uma camada de leitura/atualização com esses contratos explícitos. Não é leitor de save nem contém formato externo, IDs de entidade, contagem de componentes, flags de presença ou referências cruzadas. O payload variável de Seb/layers ainda não foi implementado.

## Validação

Exports locais `build/research/save-byte-order.tsv` (4 FDEs) e `save-native-conversion.tsv` (3 FDEs), coverage.tsv com cobertura sintática integral. Conferidos stores nativos, inversão, carga16 e extensão de sinal nos componentes já exportados.

```powershell
tools\build-desktop.cmd
```

Build Windows x64 passou, **CTest7/7**. Novo teste `animation_payload_contract` verifica fixtures explícitas de bytes, valores extremos32, truncamento e todas as65536 combinações de short, além dos256 valores de direção/booleano. Os testes exaustivos comparam o resultado numérico com a definição assinada, além de conferir reconstrução dos bytes; não se limitam a encoder e decoder concordarem entre si. Log local `build/research/animation-payload-build.log`.

Nenhum save real acessado ou original executado. Linux continua sem validação. Próximos passos: payload Seb/layers, callbacks extras e stream concreto; preservar distinção entre codec de bloco completo e comportamento de desserialização incremental. Goal ativo.
