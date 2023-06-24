.section .data

.global textures_lvl1_tim
.type textures_lvl1_tim, @object

.global light_shaft_tim
.type light_shaft_tim, @object

textures_lvl1_tim:
	.incbin "textures/TEXTURES_LVL01.TIM"

light_shaft_tim:
	.incbin "textures/SHAFT_EFFECT.tim"
	