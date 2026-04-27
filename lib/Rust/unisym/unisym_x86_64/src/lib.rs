pub fn todo_a(n: u32) -> u32 {
    n + 1
}

#[test]
fn test_add() {
    assert_eq!(todo_a(1), 2);
}
