package com.partnersintime.patcher;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * SHA-1 for the ROM identity check.
 *
 * <p>Uses the platform provider. The only reason this is wrapped is to give the
 * missing-algorithm case a message the UI can show instead of an exception name.
 */
public final class Sha1 {

    private static final char[] HEX = "0123456789abcdef".toCharArray();

    private Sha1() {
    }

    public static byte[] digest(byte[] data) {
        try {
            return MessageDigest.getInstance("SHA-1").digest(data);
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("SHA-1 unavailable on this device", e);
        }
    }

    public static String hex(byte[] digest) {
        StringBuilder sb = new StringBuilder(digest.length * 2);

        for (byte b : digest) {
            sb.append(HEX[(b >> 4) & 0x0F]);
            sb.append(HEX[b & 0x0F]);
        }
        return sb.toString();
    }
}
