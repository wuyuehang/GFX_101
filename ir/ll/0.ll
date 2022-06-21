; -S -instcombine
define i32 @test0(i32 %a) {
    %b = add i32 %a, 0
    ret i32 %b
}

define i32 @test1(i32 %a, i32 %dead) {
    ret i32 %a
}

; -S -deadargelim
define i32 @test2() {
    %1 = call i32 @test1(i32 123, i32 0)
    ret i32 %1
}