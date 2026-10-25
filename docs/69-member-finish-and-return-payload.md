# Finalização dos membros e payload de retorno

Data: 2026-10-25. Investigação estática ARM64 do APK inventariado. Endereços ELF, sem base Ghidra0x100000. Sem execução do jogo ou acesso a saves.

## OnFinish não é uma restauração genérica

MemberSystem.OnFinish37809 em0x115d618 foi exportado integralmente em build/research/member-finish. Offsets confirmados em member-finish-fields.json: members_+0x28, subMembers_+0x30. A sequência normal é:

1. Percorre members_. Para cada entidade obtém HumanComponent.data, chama CharaData.set_level com HumanComponent.level_ e copia HumanComponent.exp para CharaData.exp_ (+0x44).
2. Percorre subMembers_ e chama Entity.Destroy(true) para cada entidade.
3. Limpa members_ e depois subMembers_, incrementando versão, zerando tamanho e limpando referências usadas dos arrays de suporte.

O primeiro laço não verifica vida, resultado de batalha ou elegibilidade de EXP: copia o estado atual de todos os membros alcançados. A decisão de conceder EXP ocorreu antes, no resultado da quest. Não adicionar uma segunda concessão aqui nem tratar cópia como incremento.

CharaData.set_level38121 (0x116d5b4), em member-finish-helpers, limita entrada a[1,1200] e grava level_+0x40. A cópia de exp_ é direta e não tem clamp local. Nenhuma restauração de HP/MP aparece neste corpo; isso não exclui restauração em outros métodos da transição.

Entity.Destroy36910 (0x1131aa0) retorna se o byte+0x2c já estiver marcado. Caso contrário delega ao manager com o booleano recebido e só depois marca o byte. Não substituir por remoção imediata de um vetor; callbacks e ordem de destruição já investigados em41/50 continuam relevantes.

ReturnTown chama CleanUp antes de OnFinish, independentemente do código de saída no trecho recuperado. OnFinish não demonstra escrita de arquivo: atualiza CharaData em memória; persistência em disco permanece dependente do fluxo de save.

## Coleção usada para distribuir EXP

No relatório67, o GOT0x272fe00 da chamada genérica anterior ao laço de EXP estava pendente. Foi resolvido: usage0xc001c4c9 → spec57956 → método36997 World.GetSystem, instanciação4001 → tipo4481 MemberSystem. Portanto o ponteiro+0x28 enumerado por ShowQuestResult é members_. Não é uma lista arbitrária de todos os personagens do cadastro. Cada membro com Parameter[0].value>=1 recebe o total de EXP naquele caminho; OnFinish posteriormente copia o resultado para CharaData.

## Evento31

O GOT0x2731b18 do callback ReturnTown resolve usage0xc0019c9b → spec52813 → método36964 World.Send, instanciação2563 → tipo4258 game.QuestResult.

QuestResult é struct com battleResult, areaId e stage: offsets de metadata16/20/24 incluem cabeçalho de objeto; no payload não encaixotado os inteiros ocupam0/4/8. O callback empacota código de saída na metade inferior de x2, areaId na superior e stage em x3, coerente com os campos. O número do evento é31. Não interpretar esse payload como quantidade de moedas ou EXP.

Assinantes e efeitos do evento ainda não foram resolvidos. Uma busca por nomes encontrou métodos de construção visual de resultado, mas nenhum nome basta para provar inscrição no evento. O próximo passo deve seguir referências genéricas de AddEventHandler/dispatch, não inferir um handler pelo nome.

## Verificação e limites

Três corpos exportados com cobertura sintática; metadata, field offsets e genéricos conferidos diretamente no ELF. Não houve alteração C++ nem repetição de testes. Última validação nativa permanece10/10 do relatório66. Falta integrar finalização com ECS/callbacks e mapear assinantes do evento31 e inventário antes de declarar encerramento da quest implementado.
