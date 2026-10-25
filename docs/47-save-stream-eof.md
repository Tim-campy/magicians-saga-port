# Stream concreto do save e comportamento em EOF

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Análise estática, sem abrir saves. VAs ELF abaixo; exports Ghidra somam0x100000.

## Cadeia efetiva de leitura

LoadGame usa ByteArrayInputStream13499 (`0x1be8544`) para o bloco13, conforme46. Esse construtor aloca **System.IO.MemoryStream**, fornece o array recebido ao construtor0x1da6748 e armazena a referência em this+0x18. Identidade resolvida por GOT0x2738168→referência0x20006cf3→native type13945→metadata1300, MemoryStream.

ByteArrayInputStream não declara uma implementação própria de Read em bloco. Metadata1766, vtable20432 com14 entradas: slot11 contém0x600069e5→13554, **java.io.InputStream.Read**, VA0x1be9ba8. É o slot11 que StreamUtil.Read chama em42.

InputStream.Read lê this+0x18. Quando essa referência não é null nem WrappedSystemStream, chama o Read virtual do stream gerenciado (slot31) e transforma o resultado assim: valores>=1 são preservados; valores<1 viram **-1**. A sequência em0x1be9c40/44 é CMP1 seguido de CSINV. Não é um encaminhamento transparente de zero.

O tipo especial testado no outro ramo é java.io.WrappedSystemStream1773, resolvido pelo GOT0x273f230→0x20008c33. Ele não é o objeto MemoryStream que o construtor deste caminho aloca. Outros ramos de InputStream.Read, inclusive fallback de leitura byte a byte quando this+0x18 é null, não foram tratados como equivalentes a este caminho.

## MemoryStream.Read e leitura incompleta

MemoryStream.Read em0x1da6f74 valida buffer/offset/quantidade e estado aberto, calcula o mínimo entre quantidade pedida e bytes restantes e copia essa porção. Quando nada resta, retorna0. Com leitura parcial positiva, avança a posição e retorna quantos bytes copiou.

Composição estática para uma solicitação maior que os dados restantes:

1. MemoryStream pode devolver uma porção positiva; InputStream preserva essa quantidade.
2. StreamUtil acumula a porção e pede o restante.
3. MemoryStream no EOF devolve0; InputStream converte para-1.
4. StreamUtil encerra a tentativa e segue seu caminho de erro por quantidade insuficiente.

Portanto **o caso de zero persistente de42 não caracteriza o EOF deste caminho concreto do bloco13**. Aquela observação continua correta para uma implementação de stream que entregue zero diretamente ao loop; a resolução da cadeia agora impede generalizá-la para LoadGame. Não houve experimento de travamento ou execução do original.

As mensagens/tipos exatos da exceção, a preservação de estado em callers e todos os casos inválidos de offset ainda não foram portados. Os codecs de buffers completos43/44 continuam sem emular o stream; a descoberta apenas fecha o comportamento normal de fim de dados desta cadeia.

## Preparação do mundo: avanço e limite

AppData.LoadSystem34319 (`0x1107e8c`) foi exportado e examinado no caminho normal. Abre RecordStore0, consulta a existência de records, lê record0 quando presente, chama inicialização em0x1101a10 e SecureProperty.Load no campo AppData+0x190, depois fecha RecordStore. Não há chamada direta a World.Destroy/RemoveAll nesse trecho. Os efeitos da inicialização e caminhos de erro ainda precisam ser seguidos antes de afirmar que LoadSystem limpa o mundo.

AppData.DeserializeData34317 (`0x11077b4`) também foi exportado; chamadas diretas incluem ParseInt, ByteArrayInputStream, ReadShort/ReadInt e logging. A recuperação completa de seu laço/reflexão e dos campos escritos não foi concluída nesta rodada. Não se conclui ausência de efeitos no mundo a partir da lista de chamadas diretas. A preparação anterior a World.Deserialize continua pendente, agora com EOF resolvido separadamente.

## Validação e próximos alvos

Seleções locais `build/research/save-input-preparation.tsv`, `save-input-read.tsv` e `save-memory-read.tsv`; coverage.tsv confirma cobertura sintática integral. Foram conferidos construtor, referências de tipos, vtable herdada, conversão de retorno e ramo EOF do MemoryStream. Exportar todos os bytes de LoadSystem/DeserializeData não significa que seus efeitos completos foram reconstruídos.

Nenhum C++ alterado; não se repetiram os7 testes já aprovados. Próximo: inicialização0x1101a10, escrita de campos por DeserializeData e callers de criação/limpeza do World; depois camadas SecureProperty/RecordStore e integridade. Ainda há progresso estático útil.
