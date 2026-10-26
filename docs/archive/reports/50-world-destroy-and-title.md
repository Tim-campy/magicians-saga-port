# Destruição do mundo e limites do caminho do título

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. VAs ELF; Ghidra soma0x100000. Continuação estática de49.

## World.Destroy

World.Destroy36991 (`0x11356f4`,128 bytes) segue esta ordem normal:

1. Lê World+0x40 e chama operação genérica0x1634dbc com contexto GOT0x2730d58.
2. Lê World+0x20 e chama EntityManager.DestroyAllEntities37085 em0x1135744.
3. Lê World+0x28 e chama SystemManager.RemoveAllSystems37219 em0x1135754.
4. Lê World+0x30 e faz tail call para EventManager.RemoveAllEventHandlers37138 em0x113576c.

A primeira operação parece a limpeza da coleção, mas sua especificação genérica ainda deve ser resolvida antes de lhe atribuir contrato detalhado. As outras três chamadas foram identificadas no mapa de métodos. O efeito de DestroyAllEntities e o desligamento de callbacks constam em41; a remoção de sistemas em33.

O corpo não zera diretamente World.byte+0x38, que InitWorld consulta e Deserialize marca (32/36). Portanto não presumir que chamar Destroy sozinho restaure todas as condições de um World recém-construído. A ordem de exceções também importa: se uma etapa falha, não há finally local que garanta as seguintes.

## Busca de callers e título

Varredura de instruções BL/B alinhadas em segmentos executáveis não encontrou destino0x11356f4. Saída local `build/research/world-destroy-callers.json` é array vazio. Isso é um resultado negativo limitado a chamadas diretas nessa representação, não prova que Destroy seja inalcançável: indireções, dispatch virtual e código de outros módulos continuam fora dessa busca.

TitleForm.Init36237 (`0x1217c60`,680 bytes) chama MyFormBase.Init, configura canvas, carrega recursos, muda estado e consulta flags/save. TitleForm.Finish36247 (`0x1219d40`,68 bytes) chama método base e trata uma referência de recurso em AppData+0x138, zerando-a ao final. Nenhum dos dois corpos chama diretamente World.Destroy, DestroyAllEntities ou RemoveAllSystems. Efeitos transitivos do método base e outras transições não foram excluídos.

Essas verificações evitam inserir uma limpeza arbitrária no início de LoadGame. O caminho de Continuar recuperado em49 continua válido; falta provar a preparação do registro do World por caller anterior, override de dados ou estado inicial. A hipótese de mundo já vazio ainda é hipótese, não comportamento implementado.

## Validação e próximos alvos

Exports `build/research/world-destroy-path.tsv` e `title-cleanup.tsv`:3 FDEs,876 bytes, coverage.tsv integral. Leitura dos corpos e associação das chamadas com métodos IL2CPP; nenhum C++ alterado ou teste repetido.

Próximos passos: resolver contexto da primeira operação Destroy; seguir referências virtuais/limpeza nos métodos base/transições e GetDataList/overrides de49. Se a limpeza não puder ser estabelecida, manter a incerteza explícita enquanto avançam os outros contratos estáticos de save/ECS. Nenhum APK executado nem save acessado.
