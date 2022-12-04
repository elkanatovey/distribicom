#include "db.hpp"
namespace services{
    template<>
    void DB<seal::Plaintext>::write(const vector<uint64_t> &new_element, const int ptx_num, uint64_t offset,
                                    PIRClient *replacer_machine) {
        mtx.lock();
        memory_matrix.data[ptx_num] = replacer_machine->replace_element(memory_matrix.data[ptx_num], new_element,
                                                                       offset);
        mtx.unlock();
    }

    template<typename T>
    void
    DB<T>::write(const vector<uint64_t> &new_element, const int ptx_num, uint64_t offset, PIRClient *replacer_machine) {
        throw std::runtime_error("unsupported type");
    }

}