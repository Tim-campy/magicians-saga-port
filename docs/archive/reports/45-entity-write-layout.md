# Hooks de componentes e layout da escrita de entidades

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Continuação estática de41–44. VAs ELF; exports adicionam0x100000.

## Hooks dos três componentes portados

Vtables metadata4410 AnimationComponent (início58516),4420 DirectionComponent (58616) e4442 SebComponent (58849), todas com10 entradas, apontam seus slots6–9 para os mesmos quatro métodos herdados de IComponent:

| Slot | Método | VA | Corpo |
|---:|---|---|---|
| 6 | SerializeExtra37053 | 0x113680c | RET |
| 7 | DeserializeExtra37054 | 0x1136810 | RET |
| 8 | OnBeforeSerialize37055 | 0x1136814 | RET |
| 9 | OnDeserialized37056 | 0x1136818 | RET |

Isso fecha a pendência dos slots5/7/9 de41 para **esses três tipos concretos**: após Deserialize do payload, DeserializeExtra e OnDeserialized não consomem bytes nem alteram campos. Não generalizar a outros componentes, cujos overrides ainda podem resolver referências ou persistir dados extras.

## Bloco de componentes

BaseEntity.SerializeComponents37070 (`0x1136d50`,308 bytes) lê o array entity+0x38 e escreve seu comprimento como byte, depois percorre todos os seus índices em ordem crescente. Para cada posição:

- Null: escreve booleano false, sem payload.
- Presente: escreve true, chama OnBeforeSerialize slot8, Serialize slot4 com stream e SerializeExtra slot6 com stream.

O byte inicial representa a **quantidade de posições**, não a quantidade de componentes presentes. Para o caminho concreto de35 posições recuperado em37, é0x23. O código lê o byte baixo do comprimento ao escrever, mas o loop percorre o comprimento inteiro. Não presumir uma validação geral de limite127 nesse método; a largura/sinal no reader torna arrays grandes um contrato a investigar, embora o caminho do jogo examinado use35.

O array usado no loop é capturado em registrador após escrever o comprimento; cada posição é lida ao ser visitada, e o componente capturado é usado nos três hooks. Não foi gerado snapshot/clonagem dos componentes. Efeitos de hooks que alterem outras posições precisam ser considerados ao integrar a escrita genérica.

## Cabeçalho de uma entidade

Entity.Serialize36923 (`0x1131d04`) escreve entity+0x28 via WriteInt, depois byte+0x2c via WriteBoolean. É o par lido por Entity.Deserialize36924 (41). No BaseEntity, slot14 resolve36923 e slot16 resolve SerializeComponents37070. O seletor de tipo é escrito pelo manager antes desse cabeçalho, não pelo método Entity.Serialize.

## Sequência de EntityManager.Serialize

Serialize37094 (`0x1138890`,792 bytes) segue, no caminho normal:

1. Chama manager.OnBeforeSerialize por slot5. Em EntityManager e MyEntityManager, resolve37096 (`0x1138e80`), RET.
2. Escreve manager+0x14 como inteiro32, contador da fábrica.
3. Escreve a quantidade de entradas do dicionário manager+0x18 como inteiro32.
4. Obtém Dictionary.Values e enumera: para cada entidade, escreve seu get_type slot4 como byte e chama Serialize slot14.
5. Finaliza o primeiro enumerador, obtém Dictionary.Values novamente e inicia **outro enumerador**. Para cada entidade chama SerializeComponents slot16.

Logo o layout desta camada é:

```text
contador32, quantidade32
  [tipo8, ID32, flag8] para cada entidade da primeira passagem
  [quantidade_de_slots8, presença8/payload/extra por slot] na segunda passagem
```

Os inteiros usam a conversão big-endian comprovada em43. Os payloads conhecidos não têm SerializeExtra adicional. Esta é a camada EntityManager, não o arquivo completo de save: World ainda serializa sistemas e callers podem escrever outros dados/envelopes.

A escrita usa dois enumeradores novos, não um array de ordem congelada entre passagens. Ainda é necessário identificar a ordem/versão do Dictionary empregado e efeitos de hooks sobre seus registros. Não ordenar entidades por ID nem intercalar cabeçalho e componentes de cada entidade no port. O reader de41 registra todos os cabeçalhos antes de ler os blocos de componentes, consistente com este layout.

## Validação e próximos passos

Exports novos `build/research/component-hooks-write.tsv` e `entity-write-header.tsv`. O segundo repete SerializeComponents para confronto dos slots; isso não foi contado como evidência independente nova. Coverage.tsv confirma cobertura sintática integral. Vtables, RETs, ordem de chamadas e stores foram conferidos diretamente; nenhum save real usado e nenhum C++ alterado.

Os codecs43/44 cobrem os payloads isolados de três componentes, mas não se afirma que já leiam entidades com qualquer combinação de componentes. Próximo: factory/IDs dos demais componentes, ordem Dictionary e camada World/callers do save. Integração de arquivo completo continua dependente dessas evidências, não de uma execução Android neste momento.
