.section .data

.global skysphere_tim
.type skysphere_tim, @object

.global sun_tim
.type sun_tim, @object

.global hills_tim
.type hills_tim, @object

.global dead_tree128_tim
.type dead_tree128_tim, @object

.global grass01_tim
.type grass01_tim, @object

.global grass_tile01_tim
.type grass_tile01_tim, @object

.global textures_lvl1_tim
.type textures_lvl1_tim, @object

.global light_shaft_tim
.type light_shaft_tim, @object

skysphere_tim:
	.incbin "textures/SUNRISE_SUNSET.TIM"

sun_tim:
	.incbin "textures/SUN.TIM"

hills_tim:
	.incbin "textures/HILLS.TIM"

dead_tree128_tim:
	.incbin "textures/dead_tree.TIM"
	
grass01_tim:
	.incbin "textures/GRASS01.TIM"

grass_tile01_tim:
	.incbin "textures/GRASS_TILE.TIM"

textures_lvl1_tim:
	.incbin "textures/TEXTURES_LVL01.TIM"

light_shaft_tim:
	.incbin "textures/SHAFT_EFFECT.tim"
	