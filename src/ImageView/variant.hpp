//#pragma once
//
//#define IV_VARIANT_AS_METHOD(_type) _type as_##_type () const
//
//struct Variant
//{
//    enum VariantType
//    {
//        None,
//        VoidPointer,
//        Integer,
//        Boolean,
//    };
//
//    VariantType type;
//    union
//    {
//        void* void_pointer;
//        int   integer;
//        bool  boolean;
//    };
//
//    IV_VARIANT_AS_METHOD(int);
//    IV_VARIANT_AS_METHOD(bool);
//};
//
//#undef IV_VARIANT_AS_METHOD
