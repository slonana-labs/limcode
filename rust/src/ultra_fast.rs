/// Ultra-optimized serialization experiments
/// Goal: Beat wincode performance through advanced techniques
use std::mem::MaybeUninit;

/// Strategy 1: MaybeUninit to avoid Vec initialization overhead
#[inline(always)]
pub fn serialize_maybe_uninit(data: &[u8]) -> Vec<u8> {
    let total_len = data.len() + 8;

    unsafe {
        let mut buf: Vec<MaybeUninit<u8>> = Vec::with_capacity(total_len);
        let ptr = buf.as_mut_ptr() as *mut u8;

        // Write length directly as u64 (little-endian)
        *(ptr as *mut u64) = (data.len() as u64).to_le();

        // Write data
        std::ptr::copy_nonoverlapping(data.as_ptr(), ptr.add(8), data.len());

        // Transmute to initialized Vec
        buf.set_len(total_len);
        std::mem::transmute(buf)
    }
}

/// Strategy 2: Stack allocation for small buffers (avoid heap entirely)
#[inline(always)]
pub fn serialize_stack_small<const N: usize>(data: &[u8]) -> Vec<u8> {
    assert!(data.len() <= N, "Data too large for stack buffer");

    unsafe {
        let mut stack_buf = MaybeUninit::<[u8; N]>::uninit();
        let ptr = stack_buf.as_mut_ptr() as *mut u8;

        // Write length
        *(ptr as *mut u64) = (data.len() as u64).to_le();

        // Write data
        std::ptr::copy_nonoverlapping(data.as_ptr(), ptr.add(8), data.len());

        // Copy to Vec (single allocation + copy)
        let total_len = data.len() + 8;
        let mut vec = Vec::with_capacity(total_len);
        std::ptr::copy_nonoverlapping(ptr, vec.as_mut_ptr(), total_len);
        vec.set_len(total_len);
        vec
    }
}

/// Strategy 3: Hybrid approach - stack for small, MaybeUninit for larger
#[inline(always)]
pub fn serialize_hybrid(data: &[u8]) -> Vec<u8> {
    const STACK_THRESHOLD: usize = 120; // 128 - 8 for length prefix

    if data.len() <= STACK_THRESHOLD {
        serialize_stack_small::<128>(data)
    } else {
        serialize_maybe_uninit(data)
    }
}

/// Strategy 4: Write u64 directly without to_le_bytes intermediate
#[inline(always)]
pub fn serialize_direct_write(data: &[u8]) -> Vec<u8> {
    let total_len = data.len() + 8;
    let mut buf: Vec<u8> = Vec::with_capacity(total_len);

    unsafe {
        let ptr: *mut u8 = buf.as_mut_ptr();

        // Direct u64 write (avoids to_le_bytes() array allocation)
        std::ptr::write_unaligned(ptr as *mut u64, (data.len() as u64).to_le());

        // Copy data
        std::ptr::copy_nonoverlapping(data.as_ptr(), ptr.add(8), data.len());

        buf.set_len(total_len);
    }

    buf
}

/// Strategy 5: SIMD-optimized for small fixed sizes
#[cfg(target_arch = "x86_64")]
#[inline(always)]
pub fn serialize_simd_64(data: &[u8]) -> Vec<u8> {
    assert_eq!(data.len(), 64, "Must be exactly 64 bytes");

    unsafe {
        let mut buf: Vec<u8> = Vec::with_capacity(72);
        let ptr: *mut u8 = buf.as_mut_ptr();

        // Write length
        *(ptr as *mut u64) = 64u64.to_le();

        // SIMD copy (64 bytes = 4x 16-byte SIMD operations)
        #[cfg(target_feature = "avx")]
        {
            use std::arch::x86_64::*;
            let src = data.as_ptr();
            let dst = ptr.add(8);

            // Load and store 4x 16-byte chunks
            let v0 = _mm_loadu_si128(src as *const __m128i);
            let v1 = _mm_loadu_si128(src.add(16) as *const __m128i);
            let v2 = _mm_loadu_si128(src.add(32) as *const __m128i);
            let v3 = _mm_loadu_si128(src.add(48) as *const __m128i);

            _mm_storeu_si128(dst as *mut __m128i, v0);
            _mm_storeu_si128(dst.add(16) as *mut __m128i, v1);
            _mm_storeu_si128(dst.add(32) as *mut __m128i, v2);
            _mm_storeu_si128(dst.add(48) as *mut __m128i, v3);
        }

        #[cfg(not(target_feature = "avx"))]
        {
            std::ptr::copy_nonoverlapping(data.as_ptr(), ptr.add(8), 64);
        }

        buf.set_len(72);
        buf
    }
}

/// Strategy 6: Pre-allocated thread-local buffer pool
use std::cell::RefCell;

thread_local! {
    static BUFFER_POOL: RefCell<Vec<Vec<u8>>> = RefCell::new(Vec::with_capacity(16));
}

#[inline(always)]
pub fn serialize_pooled(data: &[u8]) -> Vec<u8> {
    let total_len = data.len() + 8;

    BUFFER_POOL.with(|pool| {
        let mut buf = pool.borrow_mut().pop().unwrap_or_else(|| Vec::new());

        buf.clear();
        buf.reserve(total_len);

        unsafe {
            let ptr: *mut u8 = buf.as_mut_ptr();
            std::ptr::write_unaligned(ptr as *mut u64, (data.len() as u64).to_le());
            std::ptr::copy_nonoverlapping(data.as_ptr(), ptr.add(8), data.len());
            buf.set_len(total_len);
        }

        buf
    })
}

pub fn return_to_pool(buf: Vec<u8>) {
    BUFFER_POOL.with(|pool| {
        let mut p = pool.borrow_mut();
        if p.len() < 16 {
            p.push(buf);
        }
    });
}
