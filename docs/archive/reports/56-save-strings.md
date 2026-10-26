# Strings do save: comprimento, encoding e localização

Data:2026-10-25. Evidência estática do ELF ARM64 identificado em55; VAs sem base Ghidra0x100000. Nenhuma execução do APK ou leitura de save real.

## Escrita comprovada

StreamUtil.WriteString14496 (`0x1c0f834`,224 bytes) consulta Config.SAVEDATA_LOCALIZE. Se ativo, chama WriteTranslate14509; em seguida, para string não nula, usa Encoding.get_UTF8 e dispatch de GetBytes. Encaminha os bytes para StreamUtil.Write14469 com flag1.

WriteTranslate retorna void (tipo nativo de retorno com kind1), e o caller conserva a referência original da string emx20. Portanto essa chamada não substitui a string por um retorno traduzido. Seus efeitos completos ainda precisam ser investigados, sem omiti-la por parecer redundante.

StreamUtil.Write14469 (`0x1c045bc`,188 bytes) com flag1 grava comprimento do array via WriteInt, depois seus bytes. Array null grava-1 e nenhum payload; array vazio grava0. Assim, o comprimento é **int32 BE em bytes**, não número de caracteres. DelayEvent já substitui args_ null por string vazia antes de WriteString (55), portanto sua escrita normal usa0 nesse caso, não-1.

Encoding.get_UTF85819 (`0x1d07230`,164 bytes) foi exportado. Políticas para UTF-16 inválido no input e substituição de caracteres ainda não foram totalmente auditadas. Não afirmar que todas as strings arbitrárias fazem round trip sem perdas.

## Leitura e detecção

ReadString14463 (`0x1c0d810`,200 bytes) chama StreamUtil.Read14432 (`0x1c03e98`,136 bytes) com flag1. Esse overload lê comprimento com ReadInt e encaminha a StreamUtil.Read14430, já examinado em42/47. Comprimento-1 resulta em null e ReadString retorna sem conversão;0 fornece bytes vazios. Não há equivalente de null/vazio para os demais comprimentos negativos comprovado aqui.

Para bytes não nulos, ReadString chama StringUtil.GetString14554 (`0x1c03f84`,260 bytes), que consulta GetCode14555 (`0x1c1a054`,2.232 bytes) e pode usar JString.New13439 com nome de encoding ou fallback JString.New13436. O tratamento seletivo de exceção nesse wrapper não foi completamente tipado; não substituir por catch-all.

GetCode começa experimentando nomes de encoding **UTF-8** e **Shift_JIS**, nessa ordem. Referências de literal nos GOT0x2732ae8/0x2739620 foram resolvidas pelo índice e bytes da metadata. Nesse trecho, decodifica candidato e procura caractere U+FFFD antes de aceitá-lo; outros ramos de detecção permanecem em análise. Portanto não tratar ReadString como simples UTF-8 incondicional só porque o escritor usa UTF-8.

JString.New13436 (`0x1b120f0`,224 bytes), caminho sem nome de encoding, usa UTF-8 e remove prefixo EF BB BF quando presente, passando offset3 e comprimento restante ao decoder. O overload13439 (`0x1b1258c`,332 bytes) contém caminho especial para Shift_JIS e escolha por nome; as rotinas internas desse caso ainda precisam ser verificadas. Resultado de comprimento zero é normalizado para a referência de string vazia nesses corpos.

## Flag de localização

O GOT0x272f5c0 resolve para Config, tipo1778. A tabela de offsets nativos associa campo7551/SAVEDATA_LOCALIZE ao offset estático0x54, exatamente o byte consultado em ReadString e WriteString.

Após a decodificação, ReadString retorna ReadTranslate14508 (`0x1c0d8d8`,1.064 bytes) quando a flag está ativa. O wrapper carrega a flag antes de chamar GetString; não reconsulta o byte depois da conversão. Valor inicial, escritores da flag e contratos completos ReadTranslate/WriteTranslate permanecem pendentes. A flag não deve ser fixada arbitrariamente em false no port.

## Validação e decisão

Exports `save-string/`: dois FDEs/424 bytes; `save-string-helpers/`: seis FDEs/3.888 bytes; `string-decode/`: três FDEs/2.788 bytes. Todos com cobertura sintática integral e hashes em coverage.tsv. Export integral não significa semântica integralmente recuperada; os trechos ainda abertos estão discriminados acima.

Não foi implementado codec completo de DelayEvent: faltam as políticas de decodificação/localização, além dos casos inválidos. O framing do bloco foi esclarecido sem introduzir um leitor UTF-8 aproximado. Nenhum C++ alterado, teste de build repetido ou comportamento dinâmico alegado. Próximo: configuração efetiva de SAVEDATA_LOCALIZE, contratos dos tradutores e restante de GetCode; também segue disponível a investigação do consumidor temporal de DelayEvent.
