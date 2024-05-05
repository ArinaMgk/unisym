// This is a template test (check)
use syst::__add;

#[test]
fn test_add() {
    assert_eq!(__add(2, 3), 5);
    assert_eq!(__add(-1, 1), 0);
    assert_eq!(__add(-2, -3), -5);
}

/* or add this in lib.rs
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = add(2, 2);
        assert_eq!(result, 4);
    }
}
*/
