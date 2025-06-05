#https://gist.github.com/malucard/037d2520d0f68b4513142bc4a14fbf14
#// hand-optimized soft float for PSX, by malucart in 2024
#// feel free to use or improve it

.set noreorder
.set nomacro
.set noat

.set abs0, $a2
.set abs1, $a3
.set leadingzeros, $t0
.set signbit, $t1
.set sig1withouthiddenzero, $t2
.set sig0, $t4
.set sig1, $t5
.set hiddenone, $t6
.set exp0, $t7
.set exp1, $t8
.set inputsxor, $t9
.set sig, $v1
.set lzcs, $30
.set lzcr, $31

#// operation: q20.12 -> float
#// 3 cycles if a == 0
#// 16 cycles if a != 0
.global qtof
qtof:
	beqz $a0, .Lret0 #// 0 is a special case because the math below can't make an exponent of 0
	sra signbit, $a0, 31 #// branch delay slot
	xor abs0, $a0, signbit
	subu abs0, abs0, signbit
	#// above: abs0 = abs($a0), sign = sign($a0)
	mtc2 abs0, lzcs #// count the zeros in abs0 to make the exponent
	sll signbit, signbit, 31 #// reduce sign to just the MSB
	li exp0, 146
	mfc2 leadingzeros, lzcr
	sll sig, abs0, 1
	sllv sig, sig, leadingzeros
	srl sig, sig, 9 #// make normalized significand while erasing MSB (becomes hidden bit)
	subu exp0, exp0, leadingzeros
	sll exp0, exp0, 23
	or $v0, signbit, exp0 #// OR the 3 elements
	jr $ra
	or $v0, $v0, sig

#// operation: float -> q20.12
#// 7 cycles if abs(a) < (1/4096)
#// 9 cycles if abs(a) >= (1/4096) && a is too large
#// 15 cycles if abs(a) >= (1/4096) && a isn't too large
.global ftoq
ftoq:
	#// extract exponent
	srl exp0, $a0, 23
	andi exp0, exp0, 0xFF
	addiu exp0, exp0, -115
	#// return 0 early if input is 0 or too small
	bltz exp0, .Lret0
	#// return INT_MIN if input is too large
	addiu $at, exp0, -30 #// branch delay slot
	bgez $at, .Lretnan
	#// extract significand
	lui hiddenone, 0x0080 #// branch delay slot
	or sig, $a0, hiddenone
	sll sig, sig, 8
	li $at, 31
	subu $at, $at, exp0
	#// turn into two's complement
	bgez $a0, .Lftoqpositive
	srl $v0, sig, $at #// branch delay slot
	jr $ra
	neg $v0, $v0 #// branch delay slot
.Lftoqpositive:
	jr $ra
	nop #// branch delay slot

.Lretnan: #// returns INT_MIN
	jr $ra
	lui $v0, 0x8000 #// branch delay slot

#// operation: float + float -> float
#// 6-35 cycles
#// 6 cycles if a == 0
#// 9 cycles if a != 0 && b == 0
#// 33 cycles if a != 0 && b != 0 && sign(a) == sign(b) && abs(a) >= abs(b)
#// 34 cycles if a != 0 && b != 0 && sign(a) == sign(b) && abs(a) < abs(b)
#// 34 cycles if a != 0 && b != 0 && sign(a) != sign(b) && abs(a) >= abs(b)
#// 35 cycles if a != 0 && b != 0 && sign(a) != sign(b) && abs(a) < abs(b)
#// ^ notably less than integer div instruction (36 cycles) (not counting call overhead)
.global __addsf3
__addsf3:
	#// shift left by 1 to get rid of the sign bit
	#// then extract the exponents by shifting right
	sll abs0, $a0, 1
	srl exp0, abs0, 24
	beqz exp0, .Lreta1 #// early return if a == 0
	sll abs1, $a1, 1 #// branch delay slot
	srl exp1, abs1, 24
	beqz exp1, .Lreta0 #// early return if b == 0

	#// extract the significands, while adding the hidden 1
	lui signbit, 0x8000 #// branch delay slot
	sll sig0, $a0, 8
	or sig0, sig0, signbit
	srl sig0, sig0, 8
	sll sig1, $a1, 8
	or sig1, sig1, signbit
	srl sig1, sig1, 8

	#// determine which input has the largest absolute value
	sltu $at, abs0, abs1
	bnez $at, .Laddrhslarger
	#// whether we're adding or subtracting depends on whether the input sign bits are different
	#// so do (a ^ b), which will be <0 (bltz) if the signs are different, or >=0 (bgez) if equal
	xor inputsxor, $a0, $a1 #// branch delay slot

	#// 		======== if here, abs0 is larger and abs1 is smaller ========
	#// set the output sign bit to that of the larger input
	and $v0, $a0, signbit
	#// adjust sig1 to work with the larger exponent by shifting it right
	subu $at, exp0, exp1
	bgez inputsxor, .Ladddontneg #// if the input signs are equal, don't negate the smaller significand
	srlv sig1, sig1, $at #// branch delay slot
	#// if the input signs are equal, subtract and skip the addition, to avoid wasting a cycle
	b .Laddmerge
	subu sig, sig0, sig1 #// branch delay slot

.Laddrhslarger:
	#// 		======== if here, abs1 is larger and abs0 is smaller ========
	#// set the output sign bit to that of the larger input
	and $v0, $a1, signbit
	#// adjust sig0 to work with the larger exponent by shifting it right
	subu $at, exp1, exp0
	move exp0, exp1
	bgez inputsxor, .Ladddontneg #// if the input signs are equal, don't negate the smaller significand
	srlv sig0, sig0, $at #// branch delay slot
	#// negate the smaller significand (only if the input signs are equal)
	neg sig0, sig0

.Ladddontneg:
	addu sig, sig0, sig1 #// here it is!!! the actual addition!!! it's so buried

.Laddmerge:
	#// back to not knowing which is larger
	mtc2 sig, lzcs
	blez sig, .Lret0 #// GTE mtc2 delay slot
	sll sig, sig, 1 #// GTE mtc2 delay slot and branch delay slot
	mfc2 leadingzeros, lzcr #// branch delay slot

	#// prepare the finalized exponent
	addiu exp0, exp0, 8 #// GTE mfc2 delay slot
	subu exp0, exp0, leadingzeros
	sll exp0, exp0, 23
	or $v0, $v0, exp0#// insert it with sign bit

	#// prepare the finalized significand
	#// sig << 1 << leadingzeros >> 9 so that the hidden 1 gets cleared
	sllv sig, sig, leadingzeros
	srl sig, sig, 9

	jr $ra
	#// $v0 had the sign bit and exponent, so this completes the float
	or $v0, $v0, sig #// branch delay slot

.Lreta0:
	jr $ra
	move $v0, $a0 #// branch delay slot

.Lreta1:
	jr $ra
	move $v0, $a1 #// branch delay slot

.Lret0:
	jr $ra
	li $v0, 0 #// branch delay slot

#// operation: float - float -> float
#// 8-37 cycles
#// same as add but with 2 extra cycles to flip b's sign bit
#// cycle sacrifice worth it(?) for saving on code size
.global __subsf3
__subsf3:
	lui signbit, 0x8000
	b __addsf3
	xor $a1, $a1, signbit #// branch delay slot

#// operation: float * float -> float
#// 13-30 cycles
#// 13 cycles if a0 == 0
#// 15 cycles if a0 != 0 && a1 == 0
#// 21 cycles if a0 != 0 && a1 != 0 && significand multiplication gives 0
#// 30 cycles if a0 != 0 && a1 != 0 && significand multiplication doesn't give 0
#// by some miracle, faster than addition? but it does sacrifice a little precision for that speed
#// there COULD be an optimization for powers of two, but the multu is fast enough that that would hurt speed
.global __mulsf3
__mulsf3:
	lui signbit, 0x8000
	#// extract significands
	#// sig0 will be 32-bit with the hidden bit as the MSB
	sll sig0, $a0, 8
	or sig0, sig0, signbit
	#// sig1 will be 20-bit, cutting off 4 low bits, which speeds up the multu
	sll sig1, $a1, 8
	or sig1, sig1, signbit
	srl sig1, sig1, 12
	#// the multu will cook for 9 cycles (not 13 because of those cut bits)
	#// thankfully, we can do stuff in the meantime
	multu sig1, sig0 #// the sigs are swapped because the left one is the one that needs to be small
	#// extract the exponents, early return if one of them is 0
	lui $at, 0x7F80
	and exp0, $a0, $at
	beqz exp0, .Lret0
	and exp1, $a1, $at
	beqz exp1, .Lret0
	#// sum the exponents
	addu exp0, exp0, exp1
	srl exp0, exp0, 23
	addiu exp0, exp0, -114

	#// XOR the input signs to determine the output sign bit later
	xor inputsxor, $a0, $a1 #// GTE mtc2 delay slot

	#// take the significand out the oven
	#// the result of a 32-bit * 20-bit multiplication is 52-bit
	#// taking just the high half yields the highest 28 bits
	#// cut the low 4 bits for performance
	mfhi sig

	#// normalize the significand
	mtc2 sig, lzcs
	beqz sig, .Lret0 #// GTE move delay slot
	and $v0, inputsxor, signbit #// GTE operation delay slot and branch delay slot
	mfc2 leadingzeros, lzcr
	sll sig, sig, 1 #// GTE move delay slot
	sllv sig, sig, leadingzeros
	srl sig, sig, 9

	subu exp0, exp0, leadingzeros
	sll exp0, exp0, 23
	or $v0, $v0, exp0

	jr $ra
	or $v0, $v0, sig

#// operation: float / float -> float
#// 13-57 cycles
#// 13 cycles if a0 == 0
#// 18 cycles if a1 is a power of two
#// 57 cycles if a0 != 0 && a1 != 1
#// note: division by zero results in zero (we do not handle infs or nans in this house)
.global __divsf3
__divsf3:
	lui signbit, 0x8000
	#// extract significands
	sll sig0, $a0, 8
	or sig0, sig0, signbit #// hidden one
	srl sig1, $a1, 8
	andi sig1withouthiddenzero, sig1, 0xFFFF
	ori sig1, sig1withouthiddenzero, 0x8000 #// hidden one
	#// the divu will cook for a crazy 36 cycles
	divu $0, sig0, sig1
	#// extract the exponents
	srl exp0, $a0, 23
	andi exp0, exp0, 0xFF
	beqz exp0, .Lret0 #// early return if a == 0
	srl exp1, $a1, 23
	#// fast return if b is a power of two
	bnez sig1withouthiddenzero, .Ldivnotpoweroftwo
	andi exp1, exp1, 0xFF #// branch delay slot
	and $v0, $a1, signbit
	addu exp1, exp1, -127
	sll exp1, exp1, 23
	subu $a0, $a0, exp1
	jr $ra
	xor $v0, $v0, $a0
.Ldivnotpoweroftwo:
	#// determine the result sign bit
	xor inputsxor, $a0, $a1
	and $v0, inputsxor, signbit
	#// subtract the exponents
	addiu exp0, exp0, 134
	subu exp0, exp0, exp1

	#// take the significand out the oven
	mflo sig
	sll sig, sig, 8

	#// normalize the significand
	mtc2 sig, lzcs
	nop #// GTE move delay slot
	nop #// GTE operation delay slot
	mfc2 leadingzeros, lzcr #// branch delay slot
	sll sig, sig, 1 #// GTE move delay slot
	sllv sig, sig, leadingzeros
	srl sig, sig, 9

	subu exp0, exp0, leadingzeros
	sll exp0, exp0, 23
	or $v0, $v0, exp0

	jr $ra
	or $v0, $v0, sig #// branch delay slot

#// so, i made 2 versions of the comparison functions. the first one runs in 4 cycles, but it uses weird stuff. the second is safe
#// operation: float spaceship?
#// 4 cycles no matter the path
.global __eqsf2, __nesf2, __ltsf2, __lesf2, __gtsf2, __gesf2
#// __eqsf2: __nesf2: __ltsf2: __lesf2: __gtsf2: __gesf2:
#// 	bgez $a0, .Lcmpa0pos
#// 	bgez $a1, .Lcmpa0nega1pos #// branch delay slot
#// 	jr $ra #// branch delay slot
#// 	subu $v0, $a1, $a0
#// .Lcmpa0nega1pos:
#// 	#// no need for jr because the one above was run in a delay slot
#// 	li $v0, -1 #// remote branch delay slot
#//
#// .Lcmpa0pos:
#// 	jr $ra #// potential remote branch delay slot
#// 	li $v0, 1 #// branch delay slot
#//
#// 	#// the jr above was run in a remote delay slot
#// 	subu $v0, $a0, $a1 #// staggered remote branch delay slot (!!!)

#// operation: float spaceship/equality
#// does not handle -0, relying on how the math has early returns for zero all over and will likely not generate a -0
#// 2 cycles
__eqsf2: __nesf2:
	jr $ra
	subu $v0, $a0, $a1 #// branch delay slot

#// operation: float spaceship
#// 6 cycles on any path
__ltsf2: __lesf2: __gtsf2: __gesf2:
	bgez $a0, .Lcmpa0pos
	nop #// branch delay slot
	bgez $a1, .Lcmpa0nega1pos
	nop #// branch delay slot
	jr $ra
	subu $v0, $a1, $a0
.Lcmpa0nega1pos:
	jr $ra
	li $v0, -1 #// branch delay slot
.Lcmpa0pos:
	bgez $a1, __eqsf2 #// .Lcmpa1nega1pos would be identical to __eqsf2
	nop #// branch delay slot
	jr $ra
	li $v0, 1 #// branch delay slot
