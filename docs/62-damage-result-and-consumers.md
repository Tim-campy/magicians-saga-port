# Resultado do dano, notificações e consumidores

Data:2026-10-25. Gameplay prioritário; investigação estática do ELF ARM64. VAs abaixo sem base adicional Ghidra0x100000. Nenhum APK executado.

## Contrato de retorno de Damage

No ramo de erro de acerto, EnemySystem.Damage retorna false depois do efeito de falha (59). No final do ramo de acerto, site0x1150a64 chama Parameter.get_value e retorna true se resultado<1. Esse getter soma value_+extraValue_ em32 bits. Portanto o teste final considera o extra, embora Parameter.Sub altere somente value_, conforme60.

O método não retorna quantidade de dano nem chama Defeat diretamente nos caminhos examinados. Não substituir o retorno por “acertou” ou por dano positivo. A guarda MyConfig.DEBUG pode pular Sub e ainda seguir para efeitos e teste final; o valor final observado pode não ter mudado nessa chamada.

## Efeitos antes da decisão de derrota

Após a subtração, o fluxo seleciona parâmetros de efeito por tabelas estáticas+0x450/+0x458, com ramificações de atributo/afinidade. Cria efeito numérico com D, pode remover/recriar ModifyAnimation no receptor inimigo, cria outro efeito na posição e envia evento15 com receptor e D no site0x11507b8. O GOT0x2731400 foi resolvido para método genérico World.Send36965, instanciação2565.

Essa notificação ocorre antes do teste final de valor. Consequentemente, alterações decorrentes de callbacks podem afetar o valor consultado ao retornar. Implementar “subtrair, decidir morte, depois notificar” mudaria essa ordem. Semântica completa dos efeitos/tabelas e assinantes do evento15 permanecem pendentes.

## Callers diretos recuperados

Busca BL/B alinhada em segmentos executáveis produziu `build/research/damage-defeat-callers.json`:

| Caller | Chamada a Damage | Uso do retorno |
|---|---|---|
|EnemySystem.OnBulletHit37627|0x114f548|Se true, chama Defeat em0x114f558; depois continua tratamento de impacto |
|EnemySystem.OnCollide37626, primeiro ramo|0x114f948|Se true, segue reação com FlyAway, animação, efeito e HumanSystem.ChangeState(4), sem Defeat nesse trecho |
|EnemySystem.OnCollide37626, segundo ramo|0x114fc28|Se true, chama Defeat em0x114fc38 |

Export de OnBulletHit (`0x114f4ac`,344 bytes) e OnCollide (`0x114f604`,1.600 bytes) em `damage-consumers/`, ambos com cobertura sintática integral. Busca direta não prova ausência de callers indiretos. As guardas completas de colisão ainda precisam ser recuperadas antes de implementar dispatch de dano.

OnBulletHit verifica componente Enemy no receptor e outros componentes do projétil; lê attack/attackAttribute do componente em sua sequência e passa como atacante referência+0x38 de outro componente. A correspondência completa dos campos/nomes ainda deve ser conferida via metadata. Não tratar o projétil em si como atacante sem seguir essa referência.

## Easing: precisão necessária

Graph.Easing15901, já exportado em60, usa float32 e chama FastMath.Sin15096 (`0x1c3d290`,116 bytes). O wrapper converte para double, chama JMath.ToRadians13414 (`0x1b11d60`) e depois uma chamada externa de matemática, retornando float32. Não foi atribuída identidade à chamada externa apenas pelo nome do wrapper.

Constantes lidas diretamente do ELF em0x627830 e0x6278ac são float32 aproximadamente3,1415927410125732 e314,1592712402344. Nos dois callers de dano, o último argumento de Easing é0, mas o cálculo do seno ainda é executado no trecho interior. Não foi substituído por uma fórmula inteira: mesmo uma interpolação sem o termo de seno deve preservar arredondamento float32 de divisão, multiplicação e soma.

Ramos comprovados do Easing: argumento de progresso negativo retorna início; progresso>=duração retorna fim; no interior aplica fórmula float, conversão para inteiro e limite conforme direção início/fim. Para os callers específicos, limites são10→100/duração50 e1→30/duração98. Ainda falta verificar o caminho matemático e a equivalência numérica nos domínios usados antes de integrar o fator.

## Estado e próxima ação

Turno adiciona a ligação entre dano e derrota e esclarece que callbacks precedem a decisão final. Nenhum C++ alterado ou teste repetido. Próximo: assinantes do evento15, guardas dos callers e fechamento numérico dos dois Easing; depois integrar cálculo, efeitos e decisão na ordem comprovada. Morte/recompensas seguem responsabilidade distinta a analisar em Defeat.
