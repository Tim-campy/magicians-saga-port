# Coleção de entidades e resolução dos filtros genéricos

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Continuação estática dos relatórios37/38. Endereços são VAs ELF, sem a base Ghidra0x100000.

## Resolução das referências genéricas

A busca de pares contagem/ponteiro no ELF encontrou duas contagens8619, iguais à quantidade de definições metadata, em0x26b5120 e0x26b5130. A vizinhança contém as tabelas estruturais abaixo. A interpretação foi confrontada com tipos já conhecidos, parâmetros e métodos, e não apenas com a ocorrência do número.

| Contagem em VA | Quantidade | Ponteiro de tabela resolvido |
|---|---:|---|
| 0x26b50e0 | 4014 | 0x2650dd0, instâncias genéricas |
| 0x26b5100 | 25389 | 0x26d61b0, tipos nativos |
| 0x26b5110 | 57978 | 0x6968d4, especificações de métodos |
| 0x26b5120 | 8619 | 0x277ece8 |
| 0x26b5130 | 8619 | 0x278fa40 |

Ponteiros são obtidos com as relocações de `tools/map_il2cpp_methods.py::Elf`; não confundir os bytes do endereço relocado com um ponteiro final. Referências codificadas têm índice `(valor & 0x1ffffffe) >> 1`; categoria6 seleciona uma especificação de método, composta de três inteiros32: definição, instância de classe e instância de método.

| GOT | Referência codificada | Especificação e interpretação |
|---|---|---|
| 0x2730fc8 | 0xc0018e17 | spec50955=(33936,-1,27): IListExtension.IsNullOrEmpty<int> |
| 0x2730e88 | 0xc001aab3 | spec54617=(49678,1063,-1): HashSet<Entity>.Add |
| 0x2730e98 | 0xc001aab9 | spec54620=(49670,1063,-1): HashSet<Entity>.Remove |
| 0x2730cd8 | 0xc001aab1 | spec54616=(49659,1063,-1): HashSet<Entity>..ctor |

Instância27 em0x26414a0 tem um argumento, ponteiro0x28153a8, kind0x08 (Int32). Instância1063 em0x2645560 tem um argumento0x28217a8, kind0x12 e definição4356, kairo.unity.ecs.Entity. Os métodos49659/49670/49678 pertencem à definição6152, System.Collections.Generic.HashSet`1; método33936 pertence a4118, IListExtension.

Confirmação independente do campo entities_ de Subset: native type3980→0x26c2fe0, kind0x15, generic class0x2855468. Esta aponta a type0x2824f08 com definição6152 e à mesma instância1063. Portanto **entities_ é HashSet<Entity>**, não List. Tipos de controle9378/12462/14221 resolvem respectivamente BaseEntity4371, IFilter4393 e MyEntityManager4445, consistentes com a análise anterior.

O helper0x12cd904 exportado no relatório38 corresponde agora ao IsNullOrEmpty<int> usado por Filter.Matches. Seu ramo null retorna true; o ramo não nulo consulta a interface contextual e compara a contagem retornada com zero. Essa resolução fecha a lacuna do predicado de listas vazias daquele relatório.

## Inserção, remoção e notificações

O wrapper Add em0x163589c resolve o contexto e faz tail call para0x1636328 (708 bytes). O corpo calcula hash, percorre a cadeia de bucket e consulta o comparador em collection+0x30. Quando hash e igualdade coincidem, retorna false em0x1636510, sem acrescentar item nem incrementar contagem/version. Quando insere, reutiliza posição livre ou expande a coleção, grava o item e incrementa contagem+0x20 e version+0x38.

Isso fecha a observação de37: Subset.Add37111 ignora o resultado de HashSet.Add. Mesmo quando a coleção retorna false por igualdade, o Subset limpa seu campo+0x38 e pode emitir OnEntityAdded. Não condicionar esse callback à inserção efetiva por convenção de outro ECS.

HashSet.Remove em0x1634fe8 (664 bytes) retorna false se não encontrar item. Em sucesso, desfaz a cadeia, limpa o ponteiro do item, mantém lista livre, decrementa contagem e incrementa version; retorna true. Subset.Remove condiciona a notificação a esse resultado, conforme37. Igualdade/hash concretos de Entity e ordem de enumeração do HashSet ainda precisam ser seguidos; não substituir automaticamente por std::unordered_set ou vetor ordenado.

O construtor da coleção em0x16347b8 (84 bytes) guarda o comparador recebido em+0x30, ou busca o default quando null; zera contagem/último índice e version, com lista livre=-1. Identidade do comparador fornecido pelo construtor de Subset permanece um próximo alvo.

## Composição e predicado portátil

Os wrappers explícitos IComponentFilter.And37158 (0x113a4c4), Or37159 (0x113a4cc) e Nor37160 (0x113a4d4) têm8 bytes cada: guardam a referência recebida respectivamente em+0x10/+0x18/+0x20 e retornam. Não concatenam critérios, copiam o array, nem invalidam hash+0x28 nesses corpos. Uma API que acumule todos os critérios de chamadas sucessivas mudaria o comportamento.

`src/core/component_filter.hpp/.cpp` implementa somente o predicado `matches_component_filter` sobre ComponentSlots concreto, com listas emprestadas:

1. Entidade null rejeita antes de consultar critérios.
2. Lista AND não nula/não vazia exige ContainsAll.
3. Lista OR não nula/não vazia exige ContainsAny.
4. Lista NOR não nula/não vazia exige ausência de ContainsAny.

Avaliação segue essa ordem e encerra no primeiro resultado que rejeita. OR vazio é ignorado, embora ContainsAny(vazio) isolado retorne false. As listas são lidas novamente em cada chamada. O módulo não implementa identidade/hash de Filter, wrappers de construção, despacho de subclasses, manutenção de grupos ou ordem do Update. Não foi adicionado um HashSet aproximado.

## Validação

Novos exports locais `build/research/collection-operations.tsv` e `collection-add-core.tsv`:7 FDEs,1496 bytes com cobertura sintática integral em coverage.tsv. As seleções usam o mesmo comando headless documentado em37. Identidades foram conferidas por tabelas nativas, relocações e metadata da cópia local do APK; nenhum comportamento em runtime foi alegado.

`tools\build-desktop.cmd`: build Windows e **CTest6/6 passaram**, incluindo teste novo `component_filter_contract`. Casos: AND/OR/NOR combinados, OR vazio, null entity, rejeição antes de critérios inválidos posteriores, índices inválidos efetivamente alcançados, alteração da lista emprestada e Change(null). Log `build/research/component-filter-build.log`. Linux e original não executados.

Próximo: comparador/hash de Entity, enumerador HashSet empregado pelo AnimationSystem, igualdade/hash dos filtros e descarte de callbacks. Ainda há avanço estático útil antes da integração completa do ECS.
