#include <iostream>
#include <type_traits>
#include <vector>
#include <cassert>
#include <cstdbool>

#include "zcm/zcm_coretypes.h"

using namespace std;

#define assert_equals(A,B) \
    __assert_equals(A, B, #A, #B, __LINE__)

template <typename T, typename F>
void __assert_equals(const T& A, const F& B,
                     const zchar_t* a, const zchar_t* b,
                     zint_t linenum)
{
    if (A != B) {
        cerr << "Assertion on line " << linenum << ": "
             << a << " != " << b << "   :   " << A << " != " << B << endl;
    }
}

static zbool_t machineIsLittleEndian()
{
    zint32_t endianess_check = 1;
    return (*(zchar_t *)&endianess_check == 1);
}

#define makeTest(V, UNIQUE, CORETYPE) \
do { \
    const decltype(V) vec = V; \
    typedef decltype(vec)::value_type primitive; \
    zint32_t nbytes = sizeof(primitive) * vec.size(); \
 \
    decltype(V) vec0 = vec; \
    decltype(V) vec1 = vec; \
    vec1.push_back(UNIQUE); \
    decltype(V) vec2 = vec; \
    decltype(V) vec3 = vec; \
    decltype(V) vec4 = vec; \
    vec4.push_back(UNIQUE); \
 \
    primitive* arr0 = (primitive*) vec0.data(); \
    primitive* arr1 = (primitive*) vec1.data(); \
    primitive* arr2 = (primitive*) vec2.data(); \
    primitive* arr3 = (primitive*) vec3.data(); \
    primitive* arr4 = (primitive*) vec4.data(); \
 \
    for (zsize_t i = 0; i < vec.size(); ++i) arr0[i] = (primitive) vec[i]; \
 \
    assert_equals((zint32_t) __ ## CORETYPE ## _encoded_array_size(arr0, vec.size()), nbytes); \
 \
    arr1[0] = UNIQUE; \
    assert_equals(__ ## CORETYPE ## _encode_array(arr1, 1 * sizeof(primitive), \
                                                  nbytes, arr0, vec.size()), \
                  nbytes); \
    assert_equals(arr1[0], UNIQUE); \
 \
 \
    assert_equals(__ ## CORETYPE ## _encode_little_endian_array(arr4, 1 * sizeof(primitive), \
                                                                nbytes, arr0, vec.size()), \
                  nbytes); \
    for (zsize_t i = 0; i < vec.size(); ++i) { \
        if (machineIsLittleEndian()) { \
            primitive tmp = 0; \
            for (zsize_t j = 0; j < sizeof(tmp); ++j) \
                ((zuint8_t*)&tmp)[j] |= ((zuint8_t*)(&arr1[i + 1]))[sizeof(tmp) - j - 1]; \
            assert_equals(tmp, vec[i]); \
            assert_equals(arr4[i + 1], vec[i]); \
        } else { \
            primitive tmp = 0; \
            for (zsize_t j = 0; j < sizeof(tmp); ++j) \
                ((zuint8_t*)&tmp)[j] |= ((zuint8_t*)(&arr4[i + 1]))[sizeof(tmp) - j - 1]; \
            assert_equals(arr1[i + 1], vec[i]); \
            assert_equals(tmp, vec[i]); \
        } \
    } \
 \
    assert_equals(__ ## CORETYPE ## _decode_array(arr1, 1 * sizeof(primitive), \
                                                  nbytes, arr2, vec.size()), \
                  nbytes); \
    for (zsize_t i = 0; i < vec.size(); ++i) assert_equals(arr2[i], vec[i]); \
 \
    assert_equals((zint32_t) __ ## CORETYPE ## _clone_array(arr2, arr3, vec.size()), nbytes); \
    for (zsize_t i = 0; i < vec.size(); ++i) assert_equals(arr3[i], vec[i]); \
 \
} while(0)

int main(int argc, char* argv[])
{
    zchar_t *a = (zchar_t*) calloc(1, 2); a[0] = 'A';
    zchar_t *b = (zchar_t*) calloc(1, 2); b[0] = 'B';
    zchar_t *c = (zchar_t*) calloc(1, 2); c[0] = 'C';
    zchar_t *d = (zchar_t*) calloc(1, 2); d[0] = 'D';
    zchar_t *e = (zchar_t*) calloc(1, 2); e[0] = 'E';

    makeTest(((vector<zuint8_t>)  {1, 2, 3, 4, 5, 6, 7, 8, 9,         10}),  11,     byte);
    makeTest(((vector<zint8_t >)  {1, 2, 3, 4, 5, 6, 7, 8, 9,         10}),  11,   int8_t);
    makeTest(((vector<zint16_t>)  {1, 2, 3, 4, 5, 6, 7, 8, 9,      31894}),  10,  int16_t);
    makeTest(((vector<zint32_t>)  {1, 2, 3, 4, 5, 6, 7, 8, 9,     301894}),  10,  int32_t);
    makeTest(((vector<zint64_t>)  {1, 2, 3, 4, 5, 6, 7, 8, 9, 3012351894}),  10,  int64_t);
    makeTest(((vector<zfloat32_t>){1, 2, 3, 4, 5, 6, 7, 8, 9,         10}),  11,    float);
    makeTest(((vector<zfloat64_t>){1, 2, 3, 4, 5, 6, 7, 8, 9,         10}),  11,   double);

    free(a);
    free(b);
    free(c);
    free(d);
    free(e);

    return 0;
}
