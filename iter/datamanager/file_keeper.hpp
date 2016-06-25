#ifndef ITER_FILE_KEEPER_HPP
#define ITER_FILE_KEEPER_HPP

#include <iter/datamanager/double_buffer.hpp>
#include <iter/datamanager/detail/loader.hpp>
#include <string>
#include <memory>
#include <mutex>
#include <type_traits>

namespace iter {

template <class LoadFunc, class Buffer =
    typename std::remove_reference <typename LoadFunc::second_argument_type>::type>
class FileKeeper : public Loader {
public:
    ~FileKeeper();

    template <class LoadFuncInit, class ...Types>
    FileKeeper(const std::string& filename,
        LoadFuncInit&& load_func_init, Types&& ...args);

    template <class ...Types>
    FileKeeper(const std::string& filename, Types&& ...args);

    bool GetBuffer(std::shared_ptr <Buffer>* ptr);
    // If the corresponding file is modified,
    // another class FileLoaderManager will call this Load() automatically.
    virtual bool Load();

private:
    typedef DoubleBuffer <Buffer> BufferMgr;
    void InitialLoad();

    std::string filename_;
    std::unique_ptr <BufferMgr> buffer_mgr_ptr_;
    std::unique_ptr <LoadFunc> load_func_ptr_;

    std::mutex mtx_;
};

} // namespace iter

#include <iter/datamanager/detail/file_keeper_impl.hpp>

#endif // ITER_FILE_KEEPER_HPP

