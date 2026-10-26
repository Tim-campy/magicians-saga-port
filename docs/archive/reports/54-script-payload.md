# Bloco de scripts: formato e codec nativo

Data:2026-10-25. Análise estática do APK1.4.0/ELF ARM64 identificado nos relatórios anteriores. Endereços são VAs ELF; exports Ghidra têm base adicional0x100000. Nenhum save real acessado ou jogo executado.

## Evidência do formato

AppData.SerializeScripts34410 (`0x110f59c`,444 bytes) recebe stream e coleção; grava Size como inteiro e percorre os elementos, convertidos para int[]. Para cada linha grava comprimento e depois todos os elementos. Usa StreamUtil.WriteInt14474 (`0x1c04258`,140 bytes), que chama EncodeUtil.IntToByteAry com flag1 e entrega cada byte ao stream. O big-endian desse conversor foi comprovado em43.

AppData.DeserializeScripts34411 (`0x110f758`,392 bytes) cria novo Vector, lê quantidade com StreamUtil.ReadInt14439 (`0x1c03458`) e retorna imediatamente se quantidade<1. Para cada linha lê comprimento, aloca int[] desse comprimento, lê seus inteiros e só então adiciona a linha à coleção. Quantidade externa negativa é aceita como coleção vazia; comprimento negativo de linha segue para alocação inválida, não representa null. O tipo exato de exceção do runtime de alocação não foi estabelecido.

Formato lógico:

```text
int32 BE quantidade
repetir quantidade vezes, apenas se quantidade > 0:
    int32 BE comprimento
    int32 BE elementos[comprimento]
```

Linhas vazias são válidas. Linhas null não têm sentinela no escritor: levam ao helper de erro. A quantidade é gravada uma vez, mas o laço do escritor volta a consultar Size; o original não faz snapshot da coleção. Não extrapolar o codec abaixo para mutações concorrentes ou callbacks de stream que alterem a coleção.

## Implementação delimitada

`src/core/script_payload.hpp/.cpp` fornece encode_script_payload e decode_script_payload, com ScriptRows como vetor de vetores int32. O escritor recebe snapshot estável e não nulo. O leitor aceita um buffer finito, preserva sinal de32 bits sem conversão unsigned→signed fora de faixa, retorna quantidade consumida e não lê o sufixo.

Limites do contrato: não é envelope de save, serializer de DelayEvent, executor de scripts ou emulação de StreamUtil incremental. Buffer truncado gera std::out_of_range; comprimento negativo gera std::length_error. Contagens são confrontadas com bytes disponíveis antes de reservar memória, portanto ordem e momento de falha podem diferir da alocação/leitura incremental do original. O leitor não expõe mutações parciais de objetos em caso de falha. Quantidade externa negativa pode ser decodificada, mas recodificar a coleção vazia emite0; não alegar round trip byte a byte dessas entradas não canônicas.

## DelayEvent é outro contrato

O ramo opcode10 de ExecuteScript34377 chama DelayEvent..ctor35499 (`0x11c9508`,220 bytes). O construtor grava os cinco argumentos inteiros em+0x10/+0x14/+0x18/+0x1c/+0x20; guarda a primeira referência em+0x28; se a segunda referência é não nula, aloca array e copia para+0x30, caso contrário mantém+0x30 null. A associação completa dos campos a unidades de tempo/IDs/argumentos e sua persistência ainda está pendente. Não confundir esse objeto com a coleção de int[] lida por DeserializeScripts.

GetDelayId34384 (`0x110caa0`,428 bytes) também foi exportado para seguir o ramo; seu contrato ainda não está encerrado neste relatório.

## Validação

Teste `tests/test_script_payload.cpp`: fixture explícita BE com três linhas incluindo vazia, opcode10, -1, INT32_MIN/MAX e0x12345678; todos os prefixos truncados; sufixo não consumido; contagens externas0/-1/INT32_MIN; comprimento negativo; contagens positivas enormes sem payload; padrões de bits positivos/negativos. Fixture sintética derivada do contrato estático, não capturada no jogo.

Comando: `tools\build-desktop.cmd`. Build Release Windows x64 e CTest **8/8 passaram**; log local `build/research/script-payload-build.log`. CMake inclui a nova biblioteca independente de plataforma e o teste script_payload_contract. Não houve validação Linux nesta rodada.

Exports novos em `script-persistence/`: três FDEs,788 bytes, cobertura sintática integral, hashes em coverage.tsv. SerializeScripts/DeserializeScripts foram relidos dos exports de53. Próximo: callers que associam o bloco ao save e semântica dos opcodes/DelayEvent, antes de integração com persistência completa ou gameplay.
