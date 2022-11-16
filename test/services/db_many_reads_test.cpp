#include "../test_utils.hpp"
#include "db.hpp"


int db_many_reads_test(int, char *[]) {
    multiplication_utils::matrix<seal::Plaintext> mat(10, 10);
    DB<seal::Plaintext> db(mat);

    {
        auto shared_mat = db.many_reads();

        // if the many_reads functions works then we shouldn't be able to lock at this time.
        assert(!db.try_write());
    }
    // if the many_reads functions works then we should be able to lock now.
    assert(db.try_write());

    {
        auto shared_mat = db.many_reads();
        shared_mat.~shared_mat();
        // if the many_reads functions works then we should be able to lock at this point in time.
        assert(db.try_write());
    }
    return 0;
}