/*
 * Reader for mods/<id>/profile.json, the launcher-side mod profile format.
 *
 * Deliberately dependency-free: org.json exists on Android but not on a desktop
 * classpath, and these classes are compiled and unit tested on the desktop. The
 * hand-written parser is a line-by-line mirror of pit_mods.c so a profile cannot
 * behave one way in the Windows launcher and another way on Android. Any change
 * here must be made there too, and tools/check_c_java_parity.py compares the
 * bytes each side produces for the same profile.
 */
package com.partnersintime.patcher;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public final class ModProfile {
    public static final String SCHEMA = "pit-mod-profile-v1";
    public static final int MAX_TRANSFORMS = 16;
    public static final int ID_MAX = 32;
    public static final int NAME_MAX = 48;
    public static final int DESCRIPTION_MAX = 200;
    public static final int VERSION_MAX = 16;
    public static final int FIELD_MAX = 24;
    public static final long NUM_MAX = 1000000000L;
    public static final long DEN_MAX = 1000000000L;
    public static final int DEFAULT_MIN = 0;
    public static final int DEFAULT_MAX = 65535;

    /** One "field"/"scale" pair naming a stat column the launcher already owns. */
    public static final class Transform {
        public final String field;
        public final long num;
        public final long den;
        public final int minValue;
        public final int maxValue;
        public final boolean enabled;

        Transform(String field, long num, long den, int minValue, int maxValue,
                  boolean enabled) {
            this.field = field;
            this.num = num;
            this.den = den;
            this.minValue = minValue;
            this.maxValue = maxValue;
            this.enabled = enabled;
        }

        /** Display form of the rational, e.g. "x11/10" or "x2". */
        public String scaleText() {
            if (den == 1L) {
                return "x" + num;
            }
            return "x" + num + "/" + den;
        }
    }

    public final String id;
    public final String name;
    public final String version;
    public final String description;
    public final Transform[] transforms;
    public final int transformCount;

    private ModProfile(String id, String name, String version, String description,
                       Transform[] transforms, int transformCount) {
        this.id = id;
        this.name = name;
        this.version = version;
        this.description = description;
        this.transforms = transforms;
        this.transformCount = transformCount;
    }

    /**
     * Half-up scaling on exact integer rationals, matching pit_mods_scale() in C
     * and make_data_profile.py's Decimal(... ROUND_HALF_UP). Doubles are avoided
     * on purpose: 1.1 * 13 must be exactly 14, not 14.000000000000002.
     */
    public static int scale(int value, long num, long den, int minValue, int maxValue) {
        long scaled = ((long) value * num + (den / 2)) / den;

        if (scaled < minValue) {
            return minValue;
        }
        if (scaled > maxValue) {
            return maxValue;
        }
        return (int) scaled;
    }

    public int scale(Transform t, int value) {
        return scale(value, t.num, t.den, t.minValue, t.maxValue);
    }

    /** Resolves a profile field name to a PatchData column, or -1 if unknown. */
    public static int fieldIndex(String field) {
        for (int i = 0; i < PatchData.FIELD_NAMES.length; i++) {
            if (PatchData.FIELD_NAMES[i].equals(field)) {
                return i;
            }
        }
        return -1;
    }

    public static ModProfile parse(String json) {
        Parser parser = new Parser(json);

        parser.expect('{');
        String schema = null;
        String id = null;
        String name = null;
        String version = "";
        String description = "";
        List<Transform> transforms = null;

        if (!parser.peek('}')) {
            do {
                String key = parser.memberKey();
                if (key.equals("schema")) {
                    schema = parser.string();
                } else if (key.equals("id")) {
                    id = parser.string();
                } else if (key.equals("name")) {
                    name = parser.string();
                } else if (key.equals("version")) {
                    version = parser.string();
                } else if (key.equals("description")) {
                    description = parser.string();
                } else if (key.equals("transforms")) {
                    transforms = parser.transforms();
                } else {
                    parser.skipValue();
                }
            } while (parser.nextSeparator());
        }
        parser.expect('}');
        parser.expectEnd();

        if (!SCHEMA.equals(schema)) {
            throw new IllegalArgumentException("profile needs \"schema\": \"" + SCHEMA + "\"");
        }
        if (id == null || id.isEmpty()) {
            throw new IllegalArgumentException("profile needs an \"id\"");
        }
        /*
         * The id is refused rather than truncated, matching the C reader: it is
         * the profile's identity, so two ids that truncate alike would be
         * indistinguishable once loaded.
         */
        if (id.length() > ID_MAX) {
            throw new IllegalArgumentException(
                    "\"id\" must be 1.." + ID_MAX + " characters");
        }
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("profile needs a \"name\"");
        }
        if (name.length() > NAME_MAX) {
            name = name.substring(0, NAME_MAX);
        }
        if (version.length() > VERSION_MAX) {
            version = version.substring(0, VERSION_MAX);
        }
        /*
         * A long description is truncated rather than refused, matching the C
         * reader: the text is shown on one clipped line in the MODS list, so the
         * extra characters were never visible anyway, and the shipped profile
         * has a description longer than this buffer holds.
         */
        if (description.length() > DESCRIPTION_MAX) {
            description = description.substring(0, DESCRIPTION_MAX);
        }
        if (transforms == null || transforms.isEmpty()) {
            throw new IllegalArgumentException("profile needs a \"transforms\" array");
        }

        int enabled = 0;
        for (int i = 0; i < transforms.size(); i++) {
            if (transforms.get(i).enabled) {
                enabled++;
            }
        }
        if (enabled == 0) {
            throw new IllegalArgumentException("no transform is enabled");
        }

        /*
         * Two transforms naming one field are refused, matching the C reader.
         * Applying both in order would scale the value twice, which reads as a
         * mistake in the profile rather than an intent this launcher can
         * express, and it would make the written bytes depend on array order.
         */
        for (int i = 0; i < transforms.size(); i++) {
            for (int j = i + 1; j < transforms.size(); j++) {
                if (transforms.get(i).field.equals(transforms.get(j).field)) {
                    throw new IllegalArgumentException("transforms " + (i + 1) + " and "
                            + (j + 1) + " both name \"" + transforms.get(i).field + "\"");
                }
            }
        }

        Transform[] array = transforms.toArray(new Transform[0]);
        return new ModProfile(id, name, version, description, array, array.length);
    }

    public static ModProfile load(File file) throws IOException {
        InputStream in = new FileInputStream(file);
        try {
            StringBuilder text = new StringBuilder();
            byte[] chunk = new byte[4096];
            int read;

            while ((read = in.read(chunk)) > 0) {
                text.append(new String(chunk, 0, read, "UTF-8"));
            }
            return parse(text.toString());
        } finally {
            in.close();
        }
    }

    @Override
    public String toString() {
        return name + " (" + id + ")";
    }

    /** One profile per mods/<dir>/profile.json, sorted so a saved choice is stable. */
    public static final class Catalog {
        private final List<ModProfile> profiles;
        public final int skipped;
        public final String error;

        private Catalog(List<ModProfile> profiles, int skipped, String error) {
            this.profiles = Collections.unmodifiableList(profiles);
            this.skipped = skipped;
            this.error = error;
        }

        public int count() {
            return profiles.size();
        }

        public ModProfile at(int index) {
            return profiles.get(index);
        }

        /** Finds a profile by declared id, which is what the catalog lists. */
        public ModProfile find(String id) {
            if (id == null || id.isEmpty()) {
                return null;
            }
            for (int i = 0; i < profiles.size(); i++) {
                if (profiles.get(i).id.equals(id)) {
                    return profiles.get(i);
                }
            }
            return null;
        }

        /**
         * Scans root for profiles. One unusable mod must not hide the others, so a
         * bad profile is counted and skipped; the first failure is reported.
         */
        public static Catalog scan(File root) {
            List<ModProfile> found = new ArrayList<ModProfile>();
            int skipped = 0;
            String error = "";
            File[] entries = root.isDirectory() ? root.listFiles() : null;

            if (entries != null) {
                for (File entry : entries) {
                    File profile = new File(entry, "profile.json");
                    if (!entry.isDirectory() || !profile.isFile()) {
                        continue;
                    }
                    try {
                        found.add(load(profile));
                    } catch (IllegalArgumentException bad) {
                        skipped++;
                        if (error.isEmpty()) {
                            error = entry.getName() + ": " + bad.getMessage();
                        }
                    } catch (IOException unreadable) {
                        skipped++;
                        if (error.isEmpty()) {
                            error = entry.getName() + ": " + unreadable.getMessage();
                        }
                    }
                }
            }

            Collections.sort(found, new Comparator<ModProfile>() {
                @Override
                public int compare(ModProfile left, ModProfile right) {
                    return left.id.compareTo(right.id);
                }
            });
            return new Catalog(found, skipped, error);
        }

        public static Catalog empty() {
            return new Catalog(new ArrayList<ModProfile>(), 0, "");
        }
    }

    /** Minimal recursive-descent JSON reader; see the class comment for scope. */
    private static final class Parser {
        private final String text;
        private int pos;

        Parser(String text) {
            this.text = text;
        }

        void fail(String message) {
            throw new IllegalArgumentException(message + " at offset " + pos);
        }

        void skipSpace() {
            while (pos < text.length()) {
                char c = text.charAt(pos);
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                    pos++;
                } else {
                    break;
                }
            }
        }

        void expect(char want) {
            skipSpace();
            if (pos >= text.length() || text.charAt(pos) != want) {
                fail("expected '" + want + "'");
            }
            pos++;
        }

        void expectEnd() {
            skipSpace();
            if (pos != text.length()) {
                fail("trailing text after the profile object");
            }
        }

        boolean peek(char want) {
            skipSpace();
            return pos < text.length() && text.charAt(pos) == want;
        }

        /** Advances over a separator and reports whether another member follows. */
        boolean nextSeparator() {
            skipSpace();
            if (pos >= text.length()) {
                fail("expected ',' or '}'");
            }
            if (text.charAt(pos) == ',') {
                pos++;
                return true;
            }
            if (text.charAt(pos) == '}') {
                return false;
            }
            fail("expected ',' or '}'");
            return false;
        }

        String string() {
            skipSpace();
            if (pos >= text.length() || text.charAt(pos) != '"') {
                fail("expected a string");
            }
            pos++;
            StringBuilder out = new StringBuilder();
            while (pos < text.length()) {
                char c = text.charAt(pos++);

                if (c == '"') {
                    return out.toString();
                }
                if (c == '\\') {
                    if (pos >= text.length()) {
                        fail("unterminated escape");
                    }
                    char esc = text.charAt(pos++);
                    if (esc == 'n') {
                        out.append('\n');
                    } else if (esc == 't') {
                        out.append('\t');
                    } else if (esc == 'u') {
                        if (pos + 4 > text.length()) {
                            fail("truncated \\u escape");
                        }
                        out.append((char) Integer.parseInt(text.substring(pos, pos + 4), 16));
                        pos += 4;
                    } else {
                        out.append(esc);
                    }
                } else {
                    out.append(c);
                }
            }
            fail("unterminated string");
            return null;
        }

        String memberKey() {
            skipSpace();
            String key = string();
            expect(':');
            return key;
        }

        /** Skips a value of a type the profile does not use, without materialising it. */
        void skipValue() {
            skipSpace();
            if (pos >= text.length()) {
                fail("expected a value");
            }
            char c = text.charAt(pos);
            if (c == '"') {
                string();
            } else if (c == '{' || c == '[') {
                char open = c;
                char close = (c == '{') ? '}' : ']';
                int depth = 0;

                while (pos < text.length()) {
                    char d = text.charAt(pos++);
                    if (d == '"') {
                        pos--;
                        string();
                        continue;
                    }
                    if (d == open) {
                        depth++;
                    } else if (d == close) {
                        depth--;
                        if (depth == 0) {
                            return;
                        }
                    }
                }
                fail("unterminated container");
            } else {
                while (pos < text.length()) {
                    char d = text.charAt(pos);
                    if (d == ',' || d == '}' || d == ']' || d == ' ' || d == '\t'
                            || d == '\r' || d == '\n') {
                        return;
                    }
                    pos++;
                }
            }
        }

        /** Reads an unsigned decimal literal as an exact rational. */
        long[] number() {
            skipSpace();
            int start = pos;

            while (pos < text.length()) {
                char c = text.charAt(pos);
                if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E'
                        || c == '+' || c == '-') {
                    pos++;
                } else {
                    break;
                }
            }
            String literal = text.substring(start, pos);
            if (literal.isEmpty()) {
                fail("expected a number");
            }
            if (literal.indexOf('e') >= 0 || literal.indexOf('E') >= 0) {
                throw new IllegalArgumentException(
                        "'" + literal + "' uses exponent notation; write it as a plain decimal");
            }
            if (literal.indexOf('-') >= 0 || literal.indexOf('+') >= 0) {
                throw new IllegalArgumentException("'" + literal + "' must be positive");
            }

            int dot = literal.indexOf('.');
            String whole = (dot < 0) ? literal : literal.substring(0, dot);
            String frac = (dot < 0) ? "" : literal.substring(dot + 1);
            if (whole.isEmpty() && frac.isEmpty()) {
                throw new IllegalArgumentException("'" + literal + "' is not a number");
            }
            for (int i = 0; i < frac.length(); i++) {
                if (frac.charAt(i) < '0' || frac.charAt(i) > '9') {
                    throw new IllegalArgumentException("'" + literal + "' is not a decimal");
                }
            }

            long num;
            long den = 1L;
            try {
                num = Long.parseLong(whole.isEmpty() ? "0" : whole);
                if (!frac.isEmpty()) {
                    long scale = 1L;
                    for (int i = 0; i < frac.length(); i++) {
                        scale *= 10L;
                    }
                    den = scale;
                    num = num * den + Long.parseLong(frac);
                }
            } catch (NumberFormatException notAFit) {
                throw new IllegalArgumentException("'" + literal + "' is out of range");
            }

            if (den > DEN_MAX || num > NUM_MAX) {
                throw new IllegalArgumentException("'" + literal + "' is out of range");
            }
            long common = greatestCommonDivisor(num, den);
            return new long[] { num / common, den / common };
        }

        private static long greatestCommonDivisor(long left, long right) {
            while (right != 0L) {
                long next = left % right;
                left = right;
                right = next;
            }
            return left == 0L ? 1L : left;
        }

        int integer(int fallback) {
            skipSpace();
            if (pos < text.length() && (text.charAt(pos) == '-' || text.charAt(pos) == '+')) {
                throw new IllegalArgumentException(
                        "'" + text.charAt(pos) + "' is not valid for an integer bound");
            }
            int start = pos;
            boolean digits = false;

            while (pos < text.length() && text.charAt(pos) >= '0'
                    && text.charAt(pos) <= '9') {
                pos++;
                digits = true;
            }
            if (!digits) {
                if (start == pos) {
                    return fallback;
                }
                fail("expected an integer");
            }
            try {
                return Integer.parseInt(text.substring(start, pos));
            } catch (NumberFormatException tooBig) {
                throw new IllegalArgumentException("'" + text.substring(start, pos)
                        + "' is out of range");
            }
        }

        boolean flag(boolean fallback) {
            skipSpace();
            if (text.startsWith("true", pos)) {
                pos += 4;
                return true;
            }
            if (text.startsWith("false", pos)) {
                pos += 5;
                return false;
            }
            if (pos >= text.length()) {
                fail("expected true or false");
            }
            fail("expected true or false");
            return fallback;
        }

        List<Transform> transforms() {
            expect('[');
            List<Transform> out = new ArrayList<Transform>();

            if (!peek(']')) {
                do {
                    out.add(transform());
                } while (nextSeparatorArray());
            }
            expect(']');
            if (out.size() > MAX_TRANSFORMS) {
                throw new IllegalArgumentException(
                        "more than " + MAX_TRANSFORMS + " transforms");
            }
            return out;
        }

        /** Comma logic inside arrays differs from objects: it closes on ']'. */
        private boolean nextSeparatorArray() {
            skipSpace();
            if (pos >= text.length()) {
                fail("expected ',' or ']'");
            }
            if (text.charAt(pos) == ',') {
                pos++;
                return true;
            }
            if (text.charAt(pos) == ']') {
                return false;
            }
            fail("expected ',' or ']'");
            return false;
        }

        Transform transform() {
            expect('{');
            String field = null;
            long[] scale = null;
            int min = DEFAULT_MIN;
            int max = DEFAULT_MAX;
            boolean enabled = true;

            if (!peek('}')) {
                do {
                    String key = memberKey();

                    if (key.equals("field")) {
                        field = string();
                    } else if (key.equals("scale")) {
                        scale = number();
                    } else if (key.equals("min")) {
                        min = integer(DEFAULT_MIN);
                    } else if (key.equals("max")) {
                        max = integer(DEFAULT_MAX);
                    } else if (key.equals("enabled")) {
                        enabled = flag(true);
                    } else {
                        skipValue();
                    }
                } while (nextSeparator());
            }
            expect('}');

            /*
             * Field and scale are validated before "enabled" so a typo is reported
             * even inside a transform the author disabled. That ordering matches
             * make_data_profile.py.
             */
            if (field == null || field.isEmpty()) {
                throw new IllegalArgumentException("transform needs a \"field\"");
            }
            if (field.length() > FIELD_MAX) {
                /*
                 * Refused rather than truncated, like the id: a truncated field
                 * name would stop matching the launcher-owned field it names.
                 */
                throw new IllegalArgumentException(
                        "transform \"field\" must be 1.." + FIELD_MAX + " characters");
            }
            if (scale == null) {
                throw new IllegalArgumentException("transform '" + field
                        + "' needs a \"scale\"");
            }
            if (scale[0] == 0L) {
                throw new IllegalArgumentException("transform '" + field
                        + "' scale must be greater than zero");
            }
            if (min < 0) {
                throw new IllegalArgumentException("transform '" + field
                        + "' min must not be negative");
            }
            if (max > DEFAULT_MAX || min > max) {
                throw new IllegalArgumentException("transform '" + field
                        + "' max must be 0.." + DEFAULT_MAX);
            }

            if (!enabled) {
                return new Transform(field, scale[0], scale[1], min, max, false);
            }
            return new Transform(field, scale[0], scale[1], min, max, true);
        }
    }
}