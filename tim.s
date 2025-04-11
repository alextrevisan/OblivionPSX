.section .data

.global textures_lvl1_tim
.type textures_lvl1_tim, @object

.global skeleton_tim
.type skeleton_tim, @object

.global light_shaft_tim
.type light_shaft_tim, @object

textures_lvl1_tim:
	.incbin "textures/TEXTURES_LVL01.TIM"

skeleton_tim:
	.incbin "textures/SKELETON.TIM"

light_shaft_tim:
	.incbin "textures/SHAFT_EFFECT.tim"
	