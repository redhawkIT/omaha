// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crypto/signature_verifier_win.h"
#include "omaha/testing/unit_test.h"

TEST(SignatureVerifierWinTest, BasicTest) {
  // The input data in this test comes from real certificates.
  //
  // tbs_certificate ("to-be-signed certificate", the part of a certificate
  // that is signed), signature_algorithm, and algorithm come from the
  // certificate of bugs.webkit.org.
  //
  // public_key_info comes from the certificate of the issuer, Go Daddy Secure
  // Certification Authority.
  //
  // The bytes in the array initializers are formatted to expose the DER
  // encoding of the ASN.1 structures.

  // The data that is signed is the following ASN.1 structure:
  //    TBSCertificate  ::=  SEQUENCE  {
  //        ...  -- omitted, not important
  //        }
  const uint8 tbs_certificate[1017] = {
    0x30, 0x82, 0x03, 0xf5,  // a SEQUENCE of length 1013 (0x3f5)
    0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x03, 0x43, 0xdd, 0x63, 0x30, 0x0d,
    0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05,
    0x00, 0x30, 0x81, 0xca, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04,
    0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55,
    0x04, 0x08, 0x13, 0x07, 0x41, 0x72, 0x69, 0x7a, 0x6f, 0x6e, 0x61, 0x31,
    0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x0a, 0x53, 0x63,
    0x6f, 0x74, 0x74, 0x73, 0x64, 0x61, 0x6c, 0x65, 0x31, 0x1a, 0x30, 0x18,
    0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x11, 0x47, 0x6f, 0x44, 0x61, 0x64,
    0x64, 0x79, 0x2e, 0x63, 0x6f, 0x6d, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e,
    0x31, 0x33, 0x30, 0x31, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x2a, 0x68,
    0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x65, 0x72, 0x74, 0x69, 0x66,
    0x69, 0x63, 0x61, 0x74, 0x65, 0x73, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64,
    0x64, 0x79, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73,
    0x69, 0x74, 0x6f, 0x72, 0x79, 0x31, 0x30, 0x30, 0x2e, 0x06, 0x03, 0x55,
    0x04, 0x03, 0x13, 0x27, 0x47, 0x6f, 0x20, 0x44, 0x61, 0x64, 0x64, 0x79,
    0x20, 0x53, 0x65, 0x63, 0x75, 0x72, 0x65, 0x20, 0x43, 0x65, 0x72, 0x74,
    0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x41, 0x75,
    0x74, 0x68, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x31, 0x11, 0x30, 0x0f, 0x06,
    0x03, 0x55, 0x04, 0x05, 0x13, 0x08, 0x30, 0x37, 0x39, 0x36, 0x39, 0x32,
    0x38, 0x37, 0x30, 0x1e, 0x17, 0x0d, 0x30, 0x38, 0x30, 0x33, 0x31, 0x38,
    0x32, 0x33, 0x33, 0x35, 0x31, 0x39, 0x5a, 0x17, 0x0d, 0x31, 0x31, 0x30,
    0x33, 0x31, 0x38, 0x32, 0x33, 0x33, 0x35, 0x31, 0x39, 0x5a, 0x30, 0x79,
    0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55,
    0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x0a,
    0x43, 0x61, 0x6c, 0x69, 0x66, 0x6f, 0x72, 0x6e, 0x69, 0x61, 0x31, 0x12,
    0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x09, 0x43, 0x75, 0x70,
    0x65, 0x72, 0x74, 0x69, 0x6e, 0x6f, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03,
    0x55, 0x04, 0x0a, 0x13, 0x0a, 0x41, 0x70, 0x70, 0x6c, 0x65, 0x20, 0x49,
    0x6e, 0x63, 0x2e, 0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x0b,
    0x13, 0x0c, 0x4d, 0x61, 0x63, 0x20, 0x4f, 0x53, 0x20, 0x46, 0x6f, 0x72,
    0x67, 0x65, 0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
    0x0c, 0x2a, 0x2e, 0x77, 0x65, 0x62, 0x6b, 0x69, 0x74, 0x2e, 0x6f, 0x72,
    0x67, 0x30, 0x81, 0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x81, 0x8d, 0x00, 0x30,
    0x81, 0x89, 0x02, 0x81, 0x81, 0x00, 0xa7, 0x62, 0x79, 0x41, 0xda, 0x28,
    0xf2, 0xc0, 0x4f, 0xe0, 0x25, 0xaa, 0xa1, 0x2e, 0x3b, 0x30, 0x94, 0xb5,
    0xc9, 0x26, 0x3a, 0x1b, 0xe2, 0xd0, 0xcc, 0xa2, 0x95, 0xe2, 0x91, 0xc0,
    0xf0, 0x40, 0x9e, 0x27, 0x6e, 0xbd, 0x6e, 0xde, 0x7c, 0xb6, 0x30, 0x5c,
    0xb8, 0x9b, 0x01, 0x2f, 0x92, 0x04, 0xa1, 0xef, 0x4a, 0xb1, 0x6c, 0xb1,
    0x7e, 0x8e, 0xcd, 0xa6, 0xf4, 0x40, 0x73, 0x1f, 0x2c, 0x96, 0xad, 0xff,
    0x2a, 0x6d, 0x0e, 0xba, 0x52, 0x84, 0x83, 0xb0, 0x39, 0xee, 0xc9, 0x39,
    0xdc, 0x1e, 0x34, 0xd0, 0xd8, 0x5d, 0x7a, 0x09, 0xac, 0xa9, 0xee, 0xca,
    0x65, 0xf6, 0x85, 0x3a, 0x6b, 0xee, 0xe4, 0x5c, 0x5e, 0xf8, 0xda, 0xd1,
    0xce, 0x88, 0x47, 0xcd, 0x06, 0x21, 0xe0, 0xb9, 0x4b, 0xe4, 0x07, 0xcb,
    0x57, 0xdc, 0xca, 0x99, 0x54, 0xf7, 0x0e, 0xd5, 0x17, 0x95, 0x05, 0x2e,
    0xe9, 0xb1, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x82, 0x01, 0xce, 0x30,
    0x82, 0x01, 0xca, 0x30, 0x09, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x02,
    0x30, 0x00, 0x30, 0x0b, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x04, 0x04, 0x03,
    0x02, 0x05, 0xa0, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x25, 0x04, 0x16,
    0x30, 0x14, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01,
    0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x02, 0x30, 0x57,
    0x06, 0x03, 0x55, 0x1d, 0x1f, 0x04, 0x50, 0x30, 0x4e, 0x30, 0x4c, 0xa0,
    0x4a, 0xa0, 0x48, 0x86, 0x46, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f,
    0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 0x73,
    0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63, 0x6f, 0x6d,
    0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f, 0x72, 0x79, 0x2f,
    0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79, 0x65, 0x78, 0x74, 0x65, 0x6e,
    0x64, 0x65, 0x64, 0x69, 0x73, 0x73, 0x75, 0x69, 0x6e, 0x67, 0x33, 0x2e,
    0x63, 0x72, 0x6c, 0x30, 0x52, 0x06, 0x03, 0x55, 0x1d, 0x20, 0x04, 0x4b,
    0x30, 0x49, 0x30, 0x47, 0x06, 0x0b, 0x60, 0x86, 0x48, 0x01, 0x86, 0xfd,
    0x6d, 0x01, 0x07, 0x17, 0x02, 0x30, 0x38, 0x30, 0x36, 0x06, 0x08, 0x2b,
    0x06, 0x01, 0x05, 0x05, 0x07, 0x02, 0x01, 0x16, 0x2a, 0x68, 0x74, 0x74,
    0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63,
    0x61, 0x74, 0x65, 0x73, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79,
    0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74,
    0x6f, 0x72, 0x79, 0x30, 0x7f, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05,
    0x07, 0x01, 0x01, 0x04, 0x73, 0x30, 0x71, 0x30, 0x23, 0x06, 0x08, 0x2b,
    0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x86, 0x17, 0x68, 0x74, 0x74,
    0x70, 0x3a, 0x2f, 0x2f, 0x6f, 0x63, 0x73, 0x70, 0x2e, 0x67, 0x6f, 0x64,
    0x61, 0x64, 0x64, 0x79, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x4a, 0x06, 0x08,
    0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x02, 0x86, 0x3e, 0x68, 0x74,
    0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69,
    0x63, 0x61, 0x74, 0x65, 0x73, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64,
    0x79, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69,
    0x74, 0x6f, 0x72, 0x79, 0x2f, 0x67, 0x64, 0x5f, 0x69, 0x6e, 0x74, 0x65,
    0x72, 0x6d, 0x65, 0x64, 0x69, 0x61, 0x74, 0x65, 0x2e, 0x63, 0x72, 0x74,
    0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x48,
    0xdf, 0x60, 0x32, 0xcc, 0x89, 0x01, 0xb6, 0xdc, 0x2f, 0xe3, 0x73, 0xb5,
    0x9c, 0x16, 0x58, 0x32, 0x68, 0xa9, 0xc3, 0x30, 0x1f, 0x06, 0x03, 0x55,
    0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0xfd, 0xac, 0x61, 0x32,
    0x93, 0x6c, 0x45, 0xd6, 0xe2, 0xee, 0x85, 0x5f, 0x9a, 0xba, 0xe7, 0x76,
    0x99, 0x68, 0xcc, 0xe7, 0x30, 0x23, 0x06, 0x03, 0x55, 0x1d, 0x11, 0x04,
    0x1c, 0x30, 0x1a, 0x82, 0x0c, 0x2a, 0x2e, 0x77, 0x65, 0x62, 0x6b, 0x69,
    0x74, 0x2e, 0x6f, 0x72, 0x67, 0x82, 0x0a, 0x77, 0x65, 0x62, 0x6b, 0x69,
    0x74, 0x2e, 0x6f, 0x72, 0x67
  };

  // RSA signature, a big integer in the big-endian byte order.
  const uint8 signature[256] = {
    0x1e, 0x6a, 0xe7, 0xe0, 0x4f, 0xe7, 0x4d, 0xd0, 0x69, 0x7c, 0xf8, 0x8f,
    0x99, 0xb4, 0x18, 0x95, 0x36, 0x24, 0x0f, 0x0e, 0xa3, 0xea, 0x34, 0x37,
    0xf4, 0x7d, 0xd5, 0x92, 0x35, 0x53, 0x72, 0x76, 0x3f, 0x69, 0xf0, 0x82,
    0x56, 0xe3, 0x94, 0x7a, 0x1d, 0x1a, 0x81, 0xaf, 0x9f, 0xc7, 0x43, 0x01,
    0x64, 0xd3, 0x7c, 0x0d, 0xc8, 0x11, 0x4e, 0x4a, 0xe6, 0x1a, 0xc3, 0x01,
    0x74, 0xe8, 0x35, 0x87, 0x5c, 0x61, 0xaa, 0x8a, 0x46, 0x06, 0xbe, 0x98,
    0x95, 0x24, 0x9e, 0x01, 0xe3, 0xe6, 0xa0, 0x98, 0xee, 0x36, 0x44, 0x56,
    0x8d, 0x23, 0x9c, 0x65, 0xea, 0x55, 0x6a, 0xdf, 0x66, 0xee, 0x45, 0xe8,
    0xa0, 0xe9, 0x7d, 0x9a, 0xba, 0x94, 0xc5, 0xc8, 0xc4, 0x4b, 0x98, 0xff,
    0x9a, 0x01, 0x31, 0x6d, 0xf9, 0x2b, 0x58, 0xe7, 0xe7, 0x2a, 0xc5, 0x4d,
    0xbb, 0xbb, 0xcd, 0x0d, 0x70, 0xe1, 0xad, 0x03, 0xf5, 0xfe, 0xf4, 0x84,
    0x71, 0x08, 0xd2, 0xbc, 0x04, 0x7b, 0x26, 0x1c, 0xa8, 0x0f, 0x9c, 0xd8,
    0x12, 0x6a, 0x6f, 0x2b, 0x67, 0xa1, 0x03, 0x80, 0x9a, 0x11, 0x0b, 0xe9,
    0xe0, 0xb5, 0xb3, 0xb8, 0x19, 0x4e, 0x0c, 0xa4, 0xd9, 0x2b, 0x3b, 0xc2,
    0xca, 0x20, 0xd3, 0x0c, 0xa4, 0xff, 0x93, 0x13, 0x1f, 0xfc, 0xba, 0x94,
    0x93, 0x8c, 0x64, 0x15, 0x2e, 0x28, 0xa9, 0x55, 0x8c, 0x2c, 0x48, 0xd3,
    0xd3, 0xc1, 0x50, 0x69, 0x19, 0xe8, 0x34, 0xd3, 0xf1, 0x04, 0x9f, 0x0a,
    0x7a, 0x21, 0x87, 0xbf, 0xb9, 0x59, 0x37, 0x2e, 0xf4, 0x71, 0xa5, 0x3e,
    0xbe, 0xcd, 0x70, 0x83, 0x18, 0xf8, 0x8a, 0x72, 0x85, 0x45, 0x1f, 0x08,
    0x01, 0x6f, 0x37, 0xf5, 0x2b, 0x7b, 0xea, 0xb9, 0x8b, 0xa3, 0xcc, 0xfd,
    0x35, 0x52, 0xdd, 0x66, 0xde, 0x4f, 0x30, 0xc5, 0x73, 0x81, 0xb6, 0xe8,
    0x3c, 0xd8, 0x48, 0x8a
  };

  // The public key is specified as the following ASN.1 structure:
  //   SubjectPublicKeyInfo  ::=  SEQUENCE  {
  //       algorithm            AlgorithmIdentifier,
  //       subjectPublicKey     BIT STRING  }
  const uint8 public_key_info[294] = {
    0x30, 0x82, 0x01, 0x22,  // a SEQUENCE of length 290 (0x122)
      // algorithm
      0x30, 0x0d,  // a SEQUENCE of length 13
        0x06, 0x09,  // an OBJECT IDENTIFIER of length 9
          0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01,
        0x05, 0x00,  // a NULL of length 0
      // subjectPublicKey
      0x03, 0x82, 0x01, 0x0f,  // a BIT STRING of length 271 (0x10f)
        0x00,  // number of unused bits
        0x30, 0x82, 0x01, 0x0a,  // a SEQUENCE of length 266 (0x10a)
          // modulus
          0x02, 0x82, 0x01, 0x01,  // an INTEGER of length 257 (0x101)
            0x00, 0xc4, 0x2d, 0xd5, 0x15, 0x8c, 0x9c, 0x26, 0x4c, 0xec,
            0x32, 0x35, 0xeb, 0x5f, 0xb8, 0x59, 0x01, 0x5a, 0xa6, 0x61,
            0x81, 0x59, 0x3b, 0x70, 0x63, 0xab, 0xe3, 0xdc, 0x3d, 0xc7,
            0x2a, 0xb8, 0xc9, 0x33, 0xd3, 0x79, 0xe4, 0x3a, 0xed, 0x3c,
            0x30, 0x23, 0x84, 0x8e, 0xb3, 0x30, 0x14, 0xb6, 0xb2, 0x87,
            0xc3, 0x3d, 0x95, 0x54, 0x04, 0x9e, 0xdf, 0x99, 0xdd, 0x0b,
            0x25, 0x1e, 0x21, 0xde, 0x65, 0x29, 0x7e, 0x35, 0xa8, 0xa9,
            0x54, 0xeb, 0xf6, 0xf7, 0x32, 0x39, 0xd4, 0x26, 0x55, 0x95,
            0xad, 0xef, 0xfb, 0xfe, 0x58, 0x86, 0xd7, 0x9e, 0xf4, 0x00,
            0x8d, 0x8c, 0x2a, 0x0c, 0xbd, 0x42, 0x04, 0xce, 0xa7, 0x3f,
            0x04, 0xf6, 0xee, 0x80, 0xf2, 0xaa, 0xef, 0x52, 0xa1, 0x69,
            0x66, 0xda, 0xbe, 0x1a, 0xad, 0x5d, 0xda, 0x2c, 0x66, 0xea,
            0x1a, 0x6b, 0xbb, 0xe5, 0x1a, 0x51, 0x4a, 0x00, 0x2f, 0x48,
            0xc7, 0x98, 0x75, 0xd8, 0xb9, 0x29, 0xc8, 0xee, 0xf8, 0x66,
            0x6d, 0x0a, 0x9c, 0xb3, 0xf3, 0xfc, 0x78, 0x7c, 0xa2, 0xf8,
            0xa3, 0xf2, 0xb5, 0xc3, 0xf3, 0xb9, 0x7a, 0x91, 0xc1, 0xa7,
            0xe6, 0x25, 0x2e, 0x9c, 0xa8, 0xed, 0x12, 0x65, 0x6e, 0x6a,
            0xf6, 0x12, 0x44, 0x53, 0x70, 0x30, 0x95, 0xc3, 0x9c, 0x2b,
            0x58, 0x2b, 0x3d, 0x08, 0x74, 0x4a, 0xf2, 0xbe, 0x51, 0xb0,
            0xbf, 0x87, 0xd0, 0x4c, 0x27, 0x58, 0x6b, 0xb5, 0x35, 0xc5,
            0x9d, 0xaf, 0x17, 0x31, 0xf8, 0x0b, 0x8f, 0xee, 0xad, 0x81,
            0x36, 0x05, 0x89, 0x08, 0x98, 0xcf, 0x3a, 0xaf, 0x25, 0x87,
            0xc0, 0x49, 0xea, 0xa7, 0xfd, 0x67, 0xf7, 0x45, 0x8e, 0x97,
            0xcc, 0x14, 0x39, 0xe2, 0x36, 0x85, 0xb5, 0x7e, 0x1a, 0x37,
            0xfd, 0x16, 0xf6, 0x71, 0x11, 0x9a, 0x74, 0x30, 0x16, 0xfe,
            0x13, 0x94, 0xa3, 0x3f, 0x84, 0x0d, 0x4f,
          // public exponent
          0x02, 0x03,  // an INTEGER of length 3
             0x01, 0x00, 0x01
  };

  // We use the signature verifier to perform four signature verification
  // tests.
  crypto::SignatureVerifierWin verifier;
  bool ok;

  // Test 1: feed all of the data to the verifier at once (a single
  // VerifyUpdate call).
  ok = verifier.VerifyInit(CALG_SHA1,
                           signature, sizeof(signature),
                           public_key_info, sizeof(public_key_info));
  EXPECT_TRUE(ok);
  verifier.VerifyUpdate(tbs_certificate, sizeof(tbs_certificate));
  ok = verifier.VerifyFinal();
  EXPECT_TRUE(ok);

  // Test 2: feed the data to the verifier in three parts (three VerifyUpdate
  // calls).
  ok = verifier.VerifyInit(CALG_SHA1,
                           signature, sizeof(signature),
                           public_key_info, sizeof(public_key_info));
  EXPECT_TRUE(ok);
  verifier.VerifyUpdate(tbs_certificate,       256);
  verifier.VerifyUpdate(tbs_certificate + 256, 256);
  verifier.VerifyUpdate(tbs_certificate + 512, sizeof(tbs_certificate) - 512);
  ok = verifier.VerifyFinal();
  EXPECT_TRUE(ok);

  // Test 3: verify the signature with incorrect data.
  uint8 bad_tbs_certificate[sizeof(tbs_certificate)];
  memcpy(bad_tbs_certificate, tbs_certificate, sizeof(tbs_certificate));
  bad_tbs_certificate[10] += 1;  // Corrupt one byte of the data.
  ok = verifier.VerifyInit(CALG_SHA1,
                           signature, sizeof(signature),
                           public_key_info, sizeof(public_key_info));
  EXPECT_TRUE(ok);
  verifier.VerifyUpdate(bad_tbs_certificate, sizeof(bad_tbs_certificate));
  ok = verifier.VerifyFinal();
  EXPECT_FALSE(ok);

  // Test 4: verify a bad signature.
  uint8 bad_signature[sizeof(signature)];
  memcpy(bad_signature, signature, sizeof(signature));
  bad_signature[10] += 1;  // Corrupt one byte of the signature.
  ok = verifier.VerifyInit(CALG_SHA1,
                           bad_signature, sizeof(bad_signature),
                           public_key_info, sizeof(public_key_info));

  // A crypto library (e.g., NSS) may detect that the signature is corrupted
  // and cause VerifyInit to return false, so it is fine for 'ok' to be false.
  if (ok) {
    verifier.VerifyUpdate(tbs_certificate, sizeof(tbs_certificate));
    ok = verifier.VerifyFinal();
    EXPECT_FALSE(ok);
  }
}

TEST(SignatureVerifierWinTest, Sha256RsaTest) {
  // SHA256 RSA signature of data256 signed with the private key corresponding
  // to sha256PublicKey. This is a big integer in the big-endian byte order.
  const uint8 signature256[256] = {
    0x2E, 0xA0, 0x36, 0x86, 0x31, 0x8C, 0x0C, 0x23, 0xC9, 0x05, 0x0A, 0x1D,
    0xDD, 0x1E, 0x46, 0xD1, 0x84, 0x80, 0x3A, 0xD5, 0xB7, 0x88, 0xB0, 0x63,
    0x34, 0xB5, 0xE2, 0xCE, 0x82, 0xA7, 0x44, 0x96, 0xBD, 0x26, 0xBF, 0xB3,
    0xBA, 0xA5, 0x0D, 0x76, 0x99, 0x45, 0x47, 0x59, 0x15, 0xB8, 0x3B, 0x68,
    0xFF, 0x60, 0xB4, 0xE3, 0x33, 0x64, 0x94, 0x7C, 0x9A, 0x81, 0x9F, 0xA7,
    0xB9, 0xF8, 0x2C, 0xE4, 0xBC, 0xC5, 0x74, 0x01, 0x10, 0xA8, 0x6C, 0xB5,
    0x62, 0xFE, 0xEB, 0x68, 0x4E, 0xB3, 0x23, 0x86, 0x44, 0x0C, 0x57, 0xFB,
    0x96, 0x89, 0xE4, 0x06, 0x47, 0x4B, 0xA4, 0x10, 0x7E, 0x2E, 0xF1, 0x89,
    0xBF, 0xA2, 0x29, 0x12, 0x61, 0x7F, 0x67, 0xD5, 0xA2, 0x65, 0x8C, 0xA5,
    0xFF, 0x9F, 0x84, 0xCF, 0x13, 0x40, 0x2E, 0xD5, 0x2E, 0xA2, 0x31, 0x5E,
    0x00, 0xDC, 0x76, 0x39, 0x6C, 0x7B, 0x17, 0xF9, 0x38, 0xD8, 0x09, 0x2C,
    0x58, 0x7B, 0x9C, 0x38, 0x8E, 0x7B, 0xCF, 0xB7, 0x1C, 0x78, 0x9F, 0x4C,
    0x71, 0x8A, 0x76, 0x6C, 0xC5, 0xD8, 0x1D, 0x07, 0x50, 0x3F, 0x24, 0x47,
    0x7E, 0x48, 0xE6, 0xE7, 0x3E, 0x25, 0xD0, 0x3F, 0x08, 0x1F, 0xC9, 0xF8,
    0x9E, 0xB2, 0xCE, 0x2E, 0xFF, 0x24, 0x4F, 0xA2, 0xE4, 0x60, 0x91, 0xCE,
    0xEB, 0x29, 0xED, 0x1F, 0x16, 0xD0, 0x42, 0xBD, 0x99, 0x5A, 0x0F, 0x69,
    0xDC, 0x8F, 0x34, 0xE8, 0xE9, 0x40, 0xCB, 0x1A, 0x0B, 0x73, 0x78, 0x2A,
    0xB9, 0xE9, 0xA6, 0x1F, 0x51, 0x20, 0x9E, 0xFD, 0xE7, 0x89, 0xB4, 0xB2,
    0xA1, 0xC3, 0x5C, 0x75, 0x8C, 0xB2, 0xC3, 0x7D, 0x1E, 0x3F, 0xD1, 0x08,
    0xA6, 0x15, 0xE5, 0xE7, 0xA5, 0x0E, 0xE4, 0x5D, 0x94, 0x9D, 0x9D, 0xD9,
    0xE5, 0x07, 0x95, 0x50, 0x32, 0x4F, 0x39, 0x8F, 0x44, 0x5D, 0x46, 0x34,
    0x7F, 0x42, 0xAA, 0x6A
  };

  // Public key corresponding to the private key used to sign data256.
  const uint8_t sha256PublicKey[] = {
    0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86,
    0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0F, 0x00,
    0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00, 0xA7, 0xB3, 0xF9,
    0x0D, 0xC7, 0xC7, 0x8D, 0x84, 0x3D, 0x4B, 0x80, 0xDD, 0x9A, 0x2F, 0xF8,
    0x69, 0xD4, 0xD1, 0x14, 0x5A, 0xCA, 0x04, 0x4B, 0x1C, 0xBC, 0x28, 0xEB,
    0x5E, 0x10, 0x01, 0x36, 0xFD, 0x81, 0xEB, 0xE4, 0x3C, 0x16, 0x40, 0xA5,
    0x8A, 0xE6, 0x08, 0xEE, 0xEF, 0x39, 0x1F, 0x6B, 0x10, 0x29, 0x50, 0x84,
    0xCE, 0xEE, 0x33, 0x5C, 0x48, 0x4A, 0x33, 0xB0, 0xC8, 0x8A, 0x66, 0x0D,
    0x10, 0x11, 0x9D, 0x6B, 0x55, 0x4C, 0x9A, 0x62, 0x40, 0x9A, 0xE2, 0xCA,
    0x21, 0x01, 0x1F, 0x10, 0x1E, 0x7B, 0xC6, 0x89, 0x94, 0xDA, 0x39, 0x69,
    0xBE, 0x27, 0x28, 0x50, 0x5E, 0xA2, 0x55, 0xB9, 0x12, 0x3C, 0x79, 0x6E,
    0xDF, 0x24, 0xBF, 0x34, 0x88, 0xF2, 0x5E, 0xD0, 0xC4, 0x06, 0xEE, 0x95,
    0x6D, 0xC2, 0x14, 0xBF, 0x51, 0x7E, 0x3F, 0x55, 0x10, 0x85, 0xCE, 0x33,
    0x8F, 0x02, 0x87, 0xFC, 0xD2, 0xDD, 0x42, 0xAF, 0x59, 0xBB, 0x69, 0x3D,
    0xBC, 0x77, 0x4B, 0x3F, 0xC7, 0x22, 0x0D, 0x5F, 0x72, 0xC7, 0x36, 0xB6,
    0x98, 0x3D, 0x03, 0xCD, 0x2F, 0x68, 0x61, 0xEE, 0xF4, 0x5A, 0xF5, 0x07,
    0xAE, 0xAE, 0x79, 0xD1, 0x1A, 0xB2, 0x38, 0xE0, 0xAB, 0x60, 0x5C, 0x0C,
    0x14, 0xFE, 0x44, 0x67, 0x2C, 0x8A, 0x08, 0x51, 0x9C, 0xCD, 0x3D, 0xDB,
    0x13, 0x04, 0x57, 0xC5, 0x85, 0xB6, 0x2A, 0x0F, 0x02, 0x46, 0x0D, 0x2D,
    0xCA, 0xE3, 0x3F, 0x84, 0x9E, 0x8B, 0x8A, 0x5F, 0xFC, 0x4D, 0xAA, 0xBE,
    0xBD, 0xE6, 0x64, 0x9F, 0x26, 0x9A, 0x2B, 0x97, 0x69, 0xA9, 0xBA, 0x0B,
    0xBD, 0x48, 0xE4, 0x81, 0x6B, 0xD4, 0x4B, 0x78, 0xE6, 0xAF, 0x95, 0x66,
    0xC1, 0x23, 0xDA, 0x23, 0x45, 0x36, 0x6E, 0x25, 0xF3, 0xC7, 0xC0, 0x61,
    0xFC, 0xEC, 0x66, 0x9D, 0x31, 0xD4, 0xD6, 0xB6, 0x36, 0xE3, 0x7F, 0x81,
    0x87, 0x02, 0x03, 0x01, 0x00, 0x01
  };

  // Data to be verified, signed with the private key corresponding to
  // sha256PublicKey.
  const uint8 data256[322] = {
    0x0A, 0xA6, 0x02, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A,
    0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82,
    0x01, 0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00,
    0x90, 0x58, 0x37, 0x8E, 0xEB, 0x21, 0x1E, 0xD2, 0x13, 0xCA, 0xB6, 0x19,
    0x4C, 0x6E, 0x0E, 0x00, 0xDB, 0x58, 0xB2, 0xFF, 0x61, 0xB1, 0x4F, 0x1F,
    0x13, 0xF7, 0x97, 0x38, 0xA7, 0x41, 0xF2, 0x15, 0x3E, 0x53, 0x87, 0x79,
    0xEF, 0x7B, 0x66, 0xD3, 0x6D, 0x39, 0x63, 0xBC, 0x31, 0xBE, 0xBF, 0x5C,
    0x4B, 0xB5, 0x00, 0x62, 0xE4, 0x36, 0xC4, 0x58, 0x04, 0xAC, 0x36, 0x06,
    0xB0, 0x26, 0xEB, 0x90, 0xEA, 0xFC, 0x17, 0x27, 0xB5, 0xED, 0xFB, 0x2B,
    0xEE, 0x33, 0xF1, 0xBE, 0x99, 0xA9, 0x0B, 0xE7, 0x0B, 0x6D, 0x4A, 0xC6,
    0xE0, 0xB1, 0x07, 0xC8, 0x49, 0xDD, 0x79, 0xCC, 0x32, 0x3E, 0x6F, 0x67,
    0x36, 0x33, 0x78, 0xAD, 0x2B, 0x15, 0x57, 0x96, 0x98, 0x40, 0xB6, 0x67,
    0xD6, 0x90, 0x8C, 0x85, 0xE7, 0x66, 0xAF, 0xC8, 0x84, 0x58, 0xBA, 0xA5,
    0x8C, 0xA7, 0x2C, 0x50, 0xBB, 0x66, 0x5E, 0x7B, 0xAE, 0x13, 0x4E, 0xF3,
    0x61, 0xDB, 0x63, 0x29, 0x53, 0xB4, 0x45, 0x88, 0xE1, 0x09, 0xE2, 0x57,
    0x5B, 0x19, 0xFF, 0x8C, 0xD2, 0x92, 0x4A, 0xEF, 0x0C, 0x86, 0xCF, 0xA1,
    0x8E, 0x68, 0x26, 0x77, 0x88, 0x1F, 0x1B, 0x82, 0x05, 0xFA, 0x01, 0xFE,
    0x5A, 0xD8, 0x47, 0xE3, 0x07, 0x68, 0x4B, 0x86, 0x92, 0x6A, 0xAB, 0x69,
    0xE9, 0x02, 0x81, 0xB9, 0x63, 0xCB, 0x43, 0x22, 0x53, 0xCB, 0xE8, 0xBD,
    0xF4, 0xFF, 0xEC, 0xB8, 0xC3, 0x4F, 0xD1, 0xBC, 0x6F, 0xC7, 0xF8, 0xBE,
    0x0B, 0xDC, 0x22, 0x40, 0xB6, 0x85, 0xC4, 0x88, 0xDF, 0xD4, 0xBD, 0xD7,
    0xAE, 0x85, 0x02, 0xEA, 0xFC, 0xD2, 0x05, 0xEC, 0xBD, 0x53, 0x42, 0x05,
    0xA5, 0xD5, 0x92, 0x09, 0x85, 0xA7, 0xC8, 0x40, 0xBD, 0x8B, 0xE2, 0xF4,
    0x96, 0x68, 0x4F, 0x80, 0x14, 0x29, 0x9A, 0x92, 0x72, 0x39, 0x03, 0x36,
    0x3C, 0x24, 0xD8, 0x19, 0x02, 0x03, 0x01, 0x00, 0x01, 0x12, 0x15, 0x62,
    0x72, 0x6F, 0x77, 0x73, 0x65, 0x72, 0x6D, 0x61, 0x6E, 0x61, 0x67, 0x65,
    0x6D, 0x65, 0x6E, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x18, 0x00
  };

  crypto::SignatureVerifierWin verifier;
  EXPECT_TRUE(verifier.VerifyInit(CALG_SHA_256,
                                  signature256, sizeof(signature256),
                                  sha256PublicKey, sizeof(sha256PublicKey)));
  verifier.VerifyUpdate(data256, sizeof(data256));
  EXPECT_TRUE(verifier.VerifyFinal());
}
