# Handler de retorno da quest

Data: 2026-10-25. Análise estática do ELF ARM64, VAs sem base Ghidra0x100000. Nenhum APK executado ou save acessado.

## Inscrição comprovada

Pesquisa nos parâmetros da metadata encontrou TownSystem.OnReturnFromQuest37993 como método declarado que recebe diretamente game.QuestResult (índice nativo15048). Isso foi usado somente para localizar o candidato. A inscrição foi confirmada em TownSystem.Init37992, VA0x1169540: constrói delegate e chama World.AddEventHandler com evento31 em0x116961c.

GOT0x2731c00 resolve usage0x600128d3 → método37993. GOT0x2731c08 resolve usage0xc0019c65 → spec52786 → método36960 AddEventHandler, instanciação2563 de QuestResult, já resolvida em69. Registro em build/research/town-return-registration.json; assembly/coverage em town-return. Isso prova um assinante, não exclusividade de handlers indiretos ou genéricos.

## Efeitos de OnReturnFromQuest

Método em0x1169b04. Copia a primeira parte do payload e compara battleResult com1. Somente nesse caso chama AppData.ExecEvent(1133/0x46d). Código1 foi ligado à morte do personagem principal em68. O conteúdo desse evento ainda não está resolvido; não presumir que seja apenas uma mensagem nem atribuir recompensa/punição sem seguir o dispatcher.

Em todos os códigos, consulta StageSystem.GetBestTotalGaveDamage37958, compara seu int32 estendido com AppData.GetHighScore em64 bits e, se o novo resultado for maior, chama SetHighScore. Trata-se do máximo dos recordes de dano entre quests já investigado em63, não do dano da última tentativa incondicionalmente. A atualização do recorde de cada quest segue suas próprias guardas.

Depois verifica AppData+0xa0; se não nulo, chama SelectForm.SetReturnFlag36010. Não usa areaId/stage do payload neste corpo. Os campos continuam fazendo parte do evento; não eliminá-los do contrato por essa observação local.

## Helpers de score e seleção

GetHighScore34442 (0x110c48c) lê o elemento8 do array em AppData+0x190 → +0x28 e converte via SecureLong. SetHighScore34443 (0x1110fc8) converte o valor para SecureLong e sobrescreve o mesmo elemento. Não há escrita direta de arquivo ou chamada de rede nesses corpos. Não concluir persistência automática nem sincronização remota; efeitos transitivos dos wrappers e salvamento são questões separadas.

SetReturnFlag em0x11f5344 grava true no byte estático SelectForm+0x32. Consumidores desse flag ainda não auditados. ExecEvent34374 é apenas encaminhamento para outra sobrecarga em0x110c668, com argumentos adicionais nulos; seu nome não descreve a semântica do evento1133.

## Estado e próximos passos

Seis corpos exportados com hashes/cobertura sintática em town-return e town-return-helpers. Nenhuma alteração C++; não repetidos testes. Último build continua10/10 do relatório70.

A cadeia comprovada agora é ReturnTown → callback → World.Send<QuestResult>(31) → inscrição TownSystem.OnReturnFromQuest, com efeitos locais descritos acima. Falta seguir evento1133 e o flag de retorno, além dos dados concretos de gameplay e integração. Não surgiu exigência de teste dinâmico como único caminho.
