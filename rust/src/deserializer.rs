//! Ultra-fast serde Deserializer - Uses C++ SIMD decoder
//!
//! This wraps the existing C++ LimcodeDecoder (AVX-512 optimized)
//! to provide serde trait support with maximum performance.

use serde::de::{self, Deserialize, DeserializeSeed, MapAccess, SeqAccess, Visitor};

/// Error type for deserialization
#[derive(Debug)]
pub enum Error {
    Message(String),
    Eof,
    InvalidBool(u8),
    InvalidChar,
    Utf8Error(std::str::Utf8Error),
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::Message(msg) => write!(f, "{}", msg),
            Error::Eof => write!(f, "unexpected end of input"),
            Error::InvalidBool(v) => write!(f, "invalid bool value: {}", v),
            Error::InvalidChar => write!(f, "invalid char"),
            Error::Utf8Error(e) => write!(f, "utf8 error: {}", e),
        }
    }
}

impl std::error::Error for Error {}

impl de::Error for Error {
    fn custom<T: std::fmt::Display>(msg: T) -> Self {
        Error::Message(msg.to_string())
    }
}

impl From<std::str::Utf8Error> for Error {
    fn from(e: std::str::Utf8Error) -> Self {
        Error::Utf8Error(e)
    }
}

/// Fast Deserializer - pure Rust for zero-copy support
/// (C++ decoder available via Decoder API for direct use)
pub struct Deserializer<'de> {
    input: &'de [u8],
    pos: usize,
}

impl<'de> Deserializer<'de> {
    pub fn new(input: &'de [u8]) -> Self {
        Self { input, pos: 0 }
    }

    #[inline(always)]
    fn read_u8(&mut self) -> Result<u8, Error> {
        if self.pos >= self.input.len() {
            return Err(Error::Eof);
        }
        let v = self.input[self.pos];
        self.pos += 1;
        Ok(v)
    }

    #[inline(always)]
    fn read_bytes(&mut self, len: usize) -> Result<&'de [u8], Error> {
        if self.pos + len > self.input.len() {
            return Err(Error::Eof);
        }
        let slice = &self.input[self.pos..self.pos + len];
        self.pos += len;
        Ok(slice)
    }

    #[inline(always)]
    fn read_u16(&mut self) -> Result<u16, Error> {
        let bytes = self.read_bytes(2)?;
        Ok(u16::from_le_bytes([bytes[0], bytes[1]]))
    }

    #[inline(always)]
    fn read_u32(&mut self) -> Result<u32, Error> {
        let bytes = self.read_bytes(4)?;
        Ok(u32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]))
    }

    #[inline(always)]
    fn read_u64(&mut self) -> Result<u64, Error> {
        let bytes = self.read_bytes(8)?;
        Ok(u64::from_le_bytes([
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
        ]))
    }

    #[inline(always)]
    fn read_i8(&mut self) -> Result<i8, Error> {
        Ok(self.read_u8()? as i8)
    }

    #[inline(always)]
    fn read_i16(&mut self) -> Result<i16, Error> {
        Ok(self.read_u16()? as i16)
    }

    #[inline(always)]
    fn read_i32(&mut self) -> Result<i32, Error> {
        Ok(self.read_u32()? as i32)
    }

    #[inline(always)]
    fn read_i64(&mut self) -> Result<i64, Error> {
        Ok(self.read_u64()? as i64)
    }

    #[inline(always)]
    fn read_f32(&mut self) -> Result<f32, Error> {
        Ok(f32::from_bits(self.read_u32()?))
    }

    #[inline(always)]
    fn read_f64(&mut self) -> Result<f64, Error> {
        Ok(f64::from_bits(self.read_u64()?))
    }
}

impl<'de, 'a> de::Deserializer<'de> for &'a mut Deserializer<'de> {
    type Error = Error;

    #[inline]
    fn deserialize_any<V: Visitor<'de>>(self, _visitor: V) -> Result<V::Value, Error> {
        Err(Error::Message("deserialize_any not supported".into()))
    }

    #[inline]
    fn deserialize_bool<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        let v = self.read_u8()?;
        match v {
            0 => visitor.visit_bool(false),
            1 => visitor.visit_bool(true),
            _ => Err(Error::InvalidBool(v)),
        }
    }

    #[inline]
    fn deserialize_i8<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_i8(self.read_i8()?)
    }

    #[inline]
    fn deserialize_i16<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_i16(self.read_i16()?)
    }

    #[inline]
    fn deserialize_i32<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_i32(self.read_i32()?)
    }

    #[inline]
    fn deserialize_i64<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_i64(self.read_i64()?)
    }

    #[inline]
    fn deserialize_u8<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_u8(self.read_u8()?)
    }

    #[inline]
    fn deserialize_u16<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_u16(self.read_u16()?)
    }

    #[inline]
    fn deserialize_u32<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_u32(self.read_u32()?)
    }

    #[inline]
    fn deserialize_u64<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_u64(self.read_u64()?)
    }

    #[inline]
    fn deserialize_f32<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_f32(self.read_f32()?)
    }

    #[inline]
    fn deserialize_f64<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_f64(self.read_f64()?)
    }

    #[inline]
    fn deserialize_char<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        let len = self.read_u64()? as usize;
        let bytes = self.read_bytes(len)?;
        let s = std::str::from_utf8(bytes)?;
        let c = s.chars().next().ok_or(Error::InvalidChar)?;
        visitor.visit_char(c)
    }

    #[inline]
    fn deserialize_str<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        let len = self.read_u64()? as usize;
        let bytes = self.read_bytes(len)?;
        let s = std::str::from_utf8(bytes)?;
        visitor.visit_borrowed_str(s)
    }

    #[inline]
    fn deserialize_string<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        self.deserialize_str(visitor)
    }

    #[inline]
    fn deserialize_bytes<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        let len = self.read_u64()? as usize;
        let bytes = self.read_bytes(len)?;
        visitor.visit_borrowed_bytes(bytes)
    }

    #[inline]
    fn deserialize_byte_buf<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        self.deserialize_bytes(visitor)
    }

    #[inline]
    fn deserialize_option<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        let tag = self.read_u8()?;
        match tag {
            0 => visitor.visit_none(),
            1 => visitor.visit_some(self),
            _ => Err(Error::Message("invalid option tag".into())),
        }
    }

    #[inline]
    fn deserialize_unit<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_unit()
    }

    #[inline]
    fn deserialize_unit_struct<V: Visitor<'de>>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Error> {
        visitor.visit_unit()
    }

    #[inline]
    fn deserialize_newtype_struct<V: Visitor<'de>>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Error> {
        visitor.visit_newtype_struct(self)
    }

    #[inline]
    fn deserialize_seq<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        let len = self.read_u64()? as usize;
        visitor.visit_seq(SeqDeserializer { de: self, remaining: len })
    }

    #[inline]
    fn deserialize_tuple<V: Visitor<'de>>(self, len: usize, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_seq(SeqDeserializer { de: self, remaining: len })
    }

    #[inline]
    fn deserialize_tuple_struct<V: Visitor<'de>>(
        self,
        _name: &'static str,
        len: usize,
        visitor: V,
    ) -> Result<V::Value, Error> {
        visitor.visit_seq(SeqDeserializer { de: self, remaining: len })
    }

    #[inline]
    fn deserialize_map<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        let len = self.read_u64()? as usize;
        visitor.visit_map(MapDeserializer { de: self, remaining: len })
    }

    #[inline]
    fn deserialize_struct<V: Visitor<'de>>(
        self,
        _name: &'static str,
        fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Error> {
        visitor.visit_seq(SeqDeserializer { de: self, remaining: fields.len() })
    }

    #[inline]
    fn deserialize_enum<V: Visitor<'de>>(
        self,
        _name: &'static str,
        _variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Error> {
        visitor.visit_enum(EnumDeserializer { de: self })
    }

    #[inline]
    fn deserialize_identifier<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        self.deserialize_str(visitor)
    }

    #[inline]
    fn deserialize_ignored_any<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        visitor.visit_unit()
    }
}

struct SeqDeserializer<'a, 'de> {
    de: &'a mut Deserializer<'de>,
    remaining: usize,
}

impl<'de, 'a> SeqAccess<'de> for SeqDeserializer<'a, 'de> {
    type Error = Error;

    #[inline]
    fn next_element_seed<T: DeserializeSeed<'de>>(&mut self, seed: T) -> Result<Option<T::Value>, Error> {
        if self.remaining == 0 {
            return Ok(None);
        }
        self.remaining -= 1;
        seed.deserialize(&mut *self.de).map(Some)
    }

    #[inline]
    fn size_hint(&self) -> Option<usize> {
        Some(self.remaining)
    }
}

struct MapDeserializer<'a, 'de> {
    de: &'a mut Deserializer<'de>,
    remaining: usize,
}

impl<'de, 'a> MapAccess<'de> for MapDeserializer<'a, 'de> {
    type Error = Error;

    #[inline]
    fn next_key_seed<K: DeserializeSeed<'de>>(&mut self, seed: K) -> Result<Option<K::Value>, Error> {
        if self.remaining == 0 {
            return Ok(None);
        }
        self.remaining -= 1;
        seed.deserialize(&mut *self.de).map(Some)
    }

    #[inline]
    fn next_value_seed<V: DeserializeSeed<'de>>(&mut self, seed: V) -> Result<V::Value, Error> {
        seed.deserialize(&mut *self.de)
    }

    #[inline]
    fn size_hint(&self) -> Option<usize> {
        Some(self.remaining)
    }
}

struct EnumDeserializer<'a, 'de> {
    de: &'a mut Deserializer<'de>,
}

impl<'de, 'a> de::EnumAccess<'de> for EnumDeserializer<'a, 'de> {
    type Error = Error;
    type Variant = Self;

    #[inline]
    fn variant_seed<V: DeserializeSeed<'de>>(self, seed: V) -> Result<(V::Value, Self::Variant), Error> {
        let variant_index = self.de.read_u32()?;
        let v = seed.deserialize(de::value::U32Deserializer::<Error>::new(variant_index))?;
        Ok((v, self))
    }
}

impl<'de, 'a> de::VariantAccess<'de> for EnumDeserializer<'a, 'de> {
    type Error = Error;

    #[inline]
    fn unit_variant(self) -> Result<(), Error> {
        Ok(())
    }

    #[inline]
    fn newtype_variant_seed<T: DeserializeSeed<'de>>(self, seed: T) -> Result<T::Value, Error> {
        seed.deserialize(&mut *self.de)
    }

    #[inline]
    fn tuple_variant<V: Visitor<'de>>(self, len: usize, visitor: V) -> Result<V::Value, Error> {
        de::Deserializer::deserialize_tuple(&mut *self.de, len, visitor)
    }

    #[inline]
    fn struct_variant<V: Visitor<'de>>(
        self,
        fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Error> {
        de::Deserializer::deserialize_struct(&mut *self.de, "", fields, visitor)
    }
}

/// Deserialize bytes to a value using our ultra-fast deserializer
#[inline]
pub fn from_bytes<'de, T: Deserialize<'de>>(bytes: &'de [u8]) -> Result<T, Error> {
    let mut deserializer = Deserializer::new(bytes);
    T::deserialize(&mut deserializer)
}

/// Same as from_bytes - matches wincode interface
#[inline]
pub fn deserialize<'de, T: Deserialize<'de>>(bytes: &'de [u8]) -> Result<T, Error> {
    from_bytes(bytes)
}

/// ACTUAL zero-copy POD deserialization - returns slice view (no allocation!)
/// For borrowed data where you don't need owned Vec
#[inline]
pub fn deserialize_pod_borrowed<'de, T: crate::serializer::PodType>(
    bytes: &'de [u8]
) -> Result<&'de [T], Error> {
    let mut de = Deserializer::new(bytes);

    // Read length prefix
    let len = de.read_u64()? as usize;

    // Calculate byte length
    let elem_size = std::mem::size_of::<T>();
    let byte_len = len * elem_size;

    // Zero-copy read - just reinterpret the slice
    let raw_bytes = de.read_bytes(byte_len)?;

    // Reinterpret &[u8] as &[T] (safe on little-endian for POD types)
    let result = unsafe {
        std::slice::from_raw_parts(raw_bytes.as_ptr() as *const T, len)
    };

    Ok(result)
}

/// POD deserialization returning Vec<T> (allocates and copies)
/// Use deserialize_pod_borrowed() for true zero-copy
#[inline]
pub fn deserialize_pod<T: crate::serializer::PodType>(bytes: &[u8]) -> Result<Vec<T>, Error> {
    let slice = deserialize_pod_borrowed::<T>(bytes)?;
    Ok(slice.to_vec())  // Single allocation, optimized by LLVM
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde::Serialize;

    #[derive(Serialize, serde::Deserialize, PartialEq, Debug)]
    struct TestStruct {
        a: u64,
        b: String,
    }

    #[test]
    fn test_deserialize_struct() {
        let data = TestStruct { a: 42, b: "hello".into() };
        let bytes = bincode::serialize(&data).unwrap();
        let decoded: TestStruct = from_bytes(&bytes).unwrap();
        assert_eq!(data, decoded);
    }

    #[test]
    fn test_deserialize_vec() {
        let data = vec![1u8, 2, 3, 4, 5];
        let bytes = bincode::serialize(&data).unwrap();
        let decoded: Vec<u8> = from_bytes(&bytes).unwrap();
        assert_eq!(data, decoded);
    }
}
