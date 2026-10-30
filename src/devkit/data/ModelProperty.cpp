#include "ModelProperty.hh"

#include <memory>
#include <vector>

namespace ra {
namespace data {

// NOTE: These static variables are not guarded by a mutex. All ModelProperties are expected to be defined statically,
// and therefore initialized in a sequential manner when the DLL is loaded. s_vProperties is a vector of _references_,
// but you can't declare a vector of reference types, so it's implemented as a vector of pointers. Don't worry about
// deleting the pointers, they should be pointing to the static ModelProperties, which aren't allocated.
// The vector is deleted with the last ModelProperty, not as a static variable: the static variables of the other files
// may be destroyed after this file's (i.e. the ModelProperties when everything is linked into one executable), and they
// would then use a deleted vector (as nothing guarantees a destroyed unique_ptr is null).
static int s_nNextKey = 1;
static std::vector<ModelPropertyBase*>* s_vProperties = nullptr;

_Use_decl_annotations_ ModelPropertyBase::ModelPropertyBase(const char* const sTypeName,
                                                            const char* const sPropertyName) :
    m_sTypeName(sTypeName),
    m_sPropertyName(sPropertyName)
{
    // ASSERT: ModelProperties are static variables and constructed when the DLL is loaded.
    // We cannot reasonably use a mutex because it cannot be locked during static initialization in WinXP.
    // ASSERT: The vector is implicitly sorted as new items are allocated higher keys and
    // added to the end of the vector.
    if (s_vProperties == nullptr)
    {
        s_vProperties = new std::vector<ModelPropertyBase*>();
        s_vProperties->reserve(128);
        s_nNextKey = 1;
    }

    m_nKey = s_nNextKey++;

    s_vProperties->push_back(this);
}

ModelPropertyBase::~ModelPropertyBase() noexcept
{
    if (!s_vProperties)
        return;

    auto iter = std::find(s_vProperties->begin(), s_vProperties->end(), this);
    if (iter != s_vProperties->end())
    {
        s_vProperties->erase(iter);

        if (s_vProperties->empty())
        {
            delete s_vProperties;
            s_vProperties = nullptr;
            s_nNextKey = 1;
        }
    }
}

const ModelPropertyBase* ModelPropertyBase::GetPropertyForKey(int nKey)
{
    if (!s_vProperties)
        return nullptr;

    ModelPropertyBase search(nKey);
    auto iter = std::lower_bound(s_vProperties->begin(), s_vProperties->end(), &search,
        [](const ModelPropertyBase* left, const ModelPropertyBase* right)
    {
        if (left == nullptr)
            return (right != nullptr);
        if (right == nullptr)
            return false;
        return left->GetKey() < right->GetKey();
    });

    return (iter == s_vProperties->end() ? nullptr : *iter);
}

} // namespace data
} // namespace ra
