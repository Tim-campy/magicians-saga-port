# Codec de payload SEB e camadas

Data: 2026-10-25. Implementação baseada no APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` e ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Evidência reutilizada de42/43; nenhum save real aberto.

## Contrato implementado

`src/core/animation_payload.hpp/.cpp` agora inclui SpritePayloadState com SpriteAnimation e optional<vector<int32_t>> para layers. Optional ausente representa array null; vetor presente vazio representa comprimento0. Não transformar nenhum deles em camada padrão[-1].

O cabeçalho ocupa5 bytes: recurso8, clip16 big-endian, frame8, comprimento8. Os campos numéricos são recuperados com sinal; os elementos subsequentes são camadas8 com sinal. Encode reduz inteiros aos bits baixos correspondentes. O limite da escrita é comprimento<127; o comprimento null é0xff.

A leitura preserva a assimetria evidenciada: comprimento0x7f=127 é aceito quando existem127 bytes de elementos, mas tentar regravar esse estado pelo serializador resulta em erro de comprimento. Comprimentos0x80–0xfe são negativos e falham; somente0xff significa null. Isso segue ReadIntArrayFromByteArray36548 (`0x1125338`) e WriteIntArrayAsByteArray36547 (`0x1125098`), usados por SebComponent37416/37417 (`0x11417a8`/`0x1141848`). O leitor original não usa a guarda<127 do escritor.

Exemplos para recurso1, clip0x1234, frame-128:

| Layers | Bytes do payload |
|---|---|
| null | `01 12 34 80 ff` |
| vazio | `01 12 34 80 00` |
| [-1,-128,127] | `01 12 34 80 03 ff 80 7f` |

## Fronteira com o stream original

decode_sprite_payload recebe buffer finito e retorna estado mais quantidade de bytes consumidos. Dados posteriores ficam fora desse payload; não são rejeitados nem interpretados. Isso permite futura composição com outros componentes sem perder os limites de cada bloco.

Truncamento do buffer gera out_of_range; comprimento negativo inválido gera length_error. Essas são decisões explícitas da API de inspeção de bloco completo, **não emulação de EOF ou identidade das exceções IL2CPP**. O codec não reproduz o loop de leitura sem progresso de42, stores parciais no objeto ou bytes já escritos antes de uma falha no serializador original. Encode constrói o bloco completo antes de entregá-lo ao caller. A integração futura com stream/objeto deve tratar essas diferenças de fronteira, sem declarar o codec sozinho equivalente ao save em runtime.

Não há envelope de save, registro de entidade, flags de presença, resolução de referências ou hooks extras implementados neste módulo. O formato variável dos layers agora está representado; fidelidade do save completo continua pendente.

## Validação

`tests/test_animation_payload.cpp` ampliado com fixtures explícitas null/vazio/camadas assinadas, todos os prefixos truncados de um payload válido, consumo correto com bytes posteriores, todos os comprimentos de leitura0–127, rejeição de escrita127, rejeição dos127 comprimentos negativos não sentinela e sentinela null com bytes seguintes.

```powershell
tools\build-desktop.cmd
```

Build Windows x64 e **CTest7/7 passaram**; o teste existente foi ampliado, não foi criado oitavo target. Log local `build/research/sprite-payload-build.log`. Permanecem os testes exaustivos16 bits e booleanos de43. Nenhum APK/save foi executado, Linux não validado nesta rodada.

Próximo: fechar hooks de componentes que cercam esses payloads na desserialização, e relacionar writer/reader externo e stream concreto. Ainda há caminhos estáticos úteis antes de testes dinâmicos.
