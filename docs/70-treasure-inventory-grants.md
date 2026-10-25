# Tesouros: descoberta, estoque e recrutamento

Data: 2026-10-25. Evidência estática do ELF ARM64 inventariado. VAs sem base adicional Ghidra0x100000. Nenhum jogo executado.

## Ordem no resultado

ShowQuestResult percorre cada tesouro, soma os bônus numéricos e processa weaponId, charaId, clothId e hatId nessa ordem. Cada campo igual a-1 é ignorado; outros valores são usados como índices com checagens de limites, não como identificadores arbitrários aceitos silenciosamente. Duplicatas na lista de tesouros são processadas novamente.

Para cada objeto consulta BaseData.get_isUnfound38046 (0x116c764), que retorna state_==0. Se verdadeiro, invoca slot8 com true. Armas/equipamentos, roupas e chapéus recebem AddStock(1); personagens recebem Recruit(). Em seguida são acrescentados pares[id,1] nas listas usadas na apresentação do resultado. Não confundir esse1 com aumento efetivo de estoque.

## Descoberta resolvida

Slot8, em class+0x1b8, foi resolvido pela vtable da metadata para BaseData.Find38052 em todos os quatro tipos: EquipmentData, ClothData, HatData e CharaData. Find em0x116c790 grava state_=1 e new_=booleano. Não há override adicional nesses tipos neste APK. Export em build/research/reward-find.

## Estoque não é contador ilimitado

EquipmentData.AddStock38251 (0x116f104), ClothData.AddStock38183 (0x116e494) e HatData.AddStock38388 (0x11718d0), exportados em treasure-grants, seguem a mesma regra:

1. Se state_!=1, chama Find(true). Esse predicado difere da checagem state_==0 feita pelo caller.
2. Soma stock_+amount em32 bits.
3. Aplica ClampMax(resultado,1), sem limite inferior local.

ClampMax34252 em0x1101854 foi exportado em stock-clamp: CMP seguido de seleção assinada do limite somente se resultado>limite. Offsets de stock_ são+0x2c em EquipmentData,+0x24 em ClothData,+0x64 em HatData; campos registrados em treasure-stock-fields.json.

Consequências: estoque1 mais1 permanece1; item já encontrado não recebe new_=true novamente por AddStock; estados diferentes de1 são convertidos para1 e marcados novos. Valores negativos e overflow não devem ser normalizados: INT_MAX+1 vira INT_MIN antes do teto, por exemplo. Não extrapolar essa regra a outras classes de inventário.

## Recrutamento

CharaData.Recruit38136 (0x116db84) grava recruit_=1 (+0x50) e chama Find(true) incondicionalmente. Portanto repetir esse caminho pode marcar novamente new_ mesmo para personagem encontrado. Não há criação de uma segunda entidade de personagem nesse corpo; cadastro e entidades de gameplay são etapas distintas.

## Implementação e testes

stage_rewards agora inclui ItemStock e add_item_stock, reproduzindo estado de descoberta, marca de novidade e soma com teto após wrap32 para os três tipos auditados. Não é ainda a integração de ShowQuestResult: busca por ID, listas de resultado, moedas, recrutamento e saída continuam com contratos separados.

Testes cobrem primeira concessão, duplicata, estado diferente de1, quantia negativa e ambos os sentidos de overflow. `tools/build-desktop.cmd`: Windows/CTest10/10 passou; log em build/research/treasure-stock-build.log. Não houve validação dinâmica ou build Linux neste ciclo.

Próximo: ligar os registros de dados aos loaders e resolver os assinantes de QuestResult antes de integrar a saída completa. Nenhuma concessão em arquivo de save foi executada.
