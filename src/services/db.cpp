#include "db.hpp"
namespace services{
    template<>
    void DB<seal::Plaintext>::write(const std::vector<std::uint64_t> &new_element, const std::uint32_t ptx_num, std::uint64_t offset,
                                    PIRClient *replacer_machine) {
        mtx.lock();
        memory_matrix.data[ptx_num] = replacer_machine->replace_element(memory_matrix.data[ptx_num], new_element,
                                                                       offset);
        mtx.unlock();
    }

    template<typename T>
    void
    DB<T>::write(const std::vector<std::uint64_t> &new_element, const std::uint32_t ptx_num, std::uint64_t offset, PIRClient *replacer_machine) {
        throw std::runtime_error("unsupported type");
    }

}