class BigInt
{
#include "prime_table.h"
    // temporary bigint pooling
    inline static std::atomic_flag lockPool = ATOMIC_FLAG_INIT;
    inline static std::atomic_flag lockFreed = ATOMIC_FLAG_INIT;
    static std::deque<BigInt> memPool;
    static std::stack<BigInt *> memFreed;
    inline static BigInt *getTemp() noexcept
    {
        while (lockFreed.test_and_set(std::memory_order_acquire))
            ;
        if (memFreed.size() == 0)
        {
            lockFreed.clear(std::memory_order_release);
            while (lockPool.test_and_set(std::memory_order_acquire))
                ;
            memPool.emplace_back();
            BigInt *ret = &*(memPool.end() - 1);
            lockPool.clear(std::memory_order_release);
            return ret;
        }
        else
        {
            BigInt *ret = memFreed.top();
            memFreed.pop();
            lockFreed.clear(std::memory_order_release);
            return ret;
        }
    }
    inline static void freeTemp(void *pthis) noexcept
    {
        while (lockFreed.test_and_set(std::memory_order_acquire))
            ;
        memFreed.push((BigInt *)pthis);
        lockFreed.clear(std::memory_order_release);
    }
    struct temp_warpper
    {
        BigInt *_p;
        inline temp_warpper(const BigInt &right)
        {
            _p = BigInt::getTemp();
            *_p = right;
        }
        inline temp_warpper(const uint32_t reset_num)
        {
            _p = BigInt::getTemp();
            _p->clear(reset_num);
        }
        inline ~temp_warpper() { BigInt::freeTemp(_p); }
        inline temp_warpper(const temp_warpper &right)
        {
            _p = BigInt::getTemp();
            _p->operator=(*right._p);
        }
        inline void operator=(const temp_warpper &right) { _p->operator=(*right._p); }
        inline BigInt *operator*() { return _p; }
        inline BigInt *operator->() { return _p; }
        inline operator BigInt &() { return *_p; }
    };

public:
    friend class io::encrypt::rsa;
    using base_t = uint32_t;
    using data_t = std::vector<base_t>;
    using const_data_t = const std::vector<base_t>;
    constexpr static int base_char = 8;
    constexpr static int base = 0xFFFFFFFF;
    constexpr static int basebitnum = 32;
    constexpr static int basebitchar = 0x1F;
    constexpr static int basebit = 5;

    friend inline BigInt operator+(const BigInt &a, const BigInt &b)
    {
        temp_warpper ca(a);
        return ca->add(b);
    }
    friend inline BigInt operator-(const BigInt &a, const BigInt &b)
    {
        temp_warpper ca(a);
        return ca->sub(b);
    }
    friend inline BigInt operator*(const BigInt &a, const BigInt &b)
    {
        if (a == (BigInt::Zero) || b == (BigInt::Zero))
            return BigInt::Zero;

        const BigInt &big = a._data.size() > b._data.size() ? a : b;
        const BigInt &small = (&big) == (&a) ? b : a;

        BigInt result(0);

        BigInt::bit bt(small);
        for (int i = bt.size() - 1; i >= 0; --i)
        {
            if (bt.at(i))
            {
                temp_warpper temp(big);
                temp->leftShift(i);
                // std::cout<<"tmp:"<<temp<<std::endl;
                result.add(temp);
                // std::cout<<"res:"<<result<<std::endl;
            }
        }
        result._isnegative = !(a._isnegative == b._isnegative);
        return result;
    }
    friend inline BigInt operator/(const BigInt &a, const BigInt &b)
    {
        assert(b != (BigInt::Zero));
        if (a.equals(b))
            return (a._isnegative == b._isnegative) ? BigInt(1) : BigInt(-1);
        else if (a.smallThan(b))
            return BigInt::Zero;
        else
        {
            BigInt result;
            temp_warpper ca(0);
            BigInt::div(a, b, result, ca);
            return result;
        }
    }
    friend inline BigInt operator%(const BigInt &a, const BigInt &b)
    {
        assert(b != (BigInt::Zero));
        if (a.equals(b))
            return BigInt::Zero;
        else if (a.smallThan(b))
            return a;
        else
        {
            BigInt ca;
            temp_warpper result(0);
            BigInt::div(a, b, result, ca);
            return ca;
        }
    }
    friend inline bool operator<(const BigInt &a, const BigInt &b)
    {
        if (a._isnegative == b._isnegative)
        {
            if (a._isnegative == false)
                return a.smallThan(b);
            else
                return !(a.smallOrEquals(b));
        }
        else
        {
            if (a._isnegative == false)
                return true;
            else
                return false;
        }
    }
    friend inline bool operator<=(const BigInt &a, const BigInt &b)
    {
        if (a._isnegative == b._isnegative)
        {
            if (a._isnegative == false)
                return a.smallOrEquals(b);
            else
                return !(a.smallThan(b));
        }
        else
        {
            if (a._isnegative == false)
                return true;
            else
                return false;
        }
    }
    friend inline bool operator==(const BigInt &a, const BigInt &b)
    {
        return a._data == b._data && a._isnegative == b._isnegative;
    }
    friend inline bool operator!=(const BigInt &a, const BigInt &b) { return !(a == b); }
    friend inline BigInt operator+(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return a + t;
    }
    friend inline BigInt operator-(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return a - t;
    }
    friend inline BigInt operator*(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return a * t;
    }
    friend inline BigInt operator/(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return a / t;
    }
    friend inline BigInt operator%(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return a % t;
    }
    friend inline bool operator<(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return a < t;
    }
    friend inline bool operator<=(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return a <= t;
    }
    friend inline bool operator==(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return a == t;
    }
    friend inline bool operator!=(const BigInt &a, const uint32_t b)
    {
        temp_warpper t(b);
        return !(a == t);
    }
    inline operator bool()
    {
        for (auto i : _data)
        {
            if (i != 0)
                return true;
        }
        return false;
    }

    friend inline std::ostream &operator<<(std::ostream &out, const BigInt &a)
    {
        static char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
        if (a._isnegative)
            out << "-";
        BigInt::base_t T = 0x0F;
        std::string str;
        for (BigInt::data_t::const_iterator it = a._data.begin(); it != a._data.end(); ++it)
        {
            BigInt::base_t ch = (*it);
            for (int j = 0; j < BigInt::base_char; ++j)
            {
                str.push_back(hex[ch & (T)]);
                ch = ch >> 4;
            }
        }
        reverse(str.begin(), str.end());
        out << str;
        return out;
    }
    friend inline BigInt operator<<(const BigInt &a, unsigned int n)
    {
        temp_warpper ca(a);
        return ca->leftShift(n);
    }

    inline BigInt &trim()
    {
        int count = 0;
        for (data_t::reverse_iterator it = _data.rbegin(); it != _data.rend(); ++it)
            if ((*it) == 0)
                ++count;
            else
                break;
        if (count == _data.size())
            --count;
        for (int i = 0; i < count; ++i)
            _data.pop_back();
        return *this;
    }
    friend class bit;
    class bit
    {
    public:
        inline std::size_t size() { return _size; };
        inline bool at(std::size_t i)
        {
            std::size_t index = i >> (BigInt::basebit);
            std::size_t off = i & (BigInt::basebitchar);
            BigInt::base_t t = _bitvec[index];
            return (t & (1 << off));
        }
        inline bit(const BigInt &ba)
        {
            _bitvec = ba._data;
            BigInt::base_t a = _bitvec[_bitvec.size() - 1];
            _size = _bitvec.size() << (BigInt::basebit);
            BigInt::base_t t = 1 << (BigInt::basebitnum - 1);

            if (a == 0)
                _size -= (BigInt::basebitnum);
            else
            {
                while (!(a & t))
                {
                    --_size;
                    t = t >> 1;
                }
            }
        }

    private:
        std::vector<base_t> _bitvec;
        std::size_t _size;
    };
    inline BigInt moden(const BigInt &exp, const BigInt &p) const
    {
        // tradictional modular multiplication way
        BigInt::bit t(exp);

        BigInt d(1);
        for (int i = t.size() - 1; i >= 0; --i)
        {
            d = (d * d) % p; // costs lots of time
            if (t.at(i))
                d = (d * (*this)) % p;
        }
        return d;
    }
    inline BigInt extendEuclid(const BigInt &m) const
    {
        assert(m._isnegative == false); // m is positive
        BigInt a[3], b[3], t[3];
        a[0] = 1;
        a[1] = 0;
        a[2] = m;
        b[0] = 0;
        b[1] = 1;
        b[2] = *this;
        if (b[2] == BigInt::Zero || b[2] == BigInt::One)
            return b[2];

        while (true)
        {
            if (b[2] == BigInt::One)
            {
                if (b[1]._isnegative == true) // nega
                    b[1] = (b[1] % m + m) % m;
                return b[1];
            }

            BigInt q = a[2] / b[2];
            for (int i = 0; i < 3; ++i)
            {
                t[i] = a[i] - q * b[i];
                a[i] = b[i];
                b[i] = t[i];
            }
        }
    }

    inline BigInt() : _isnegative(false) { _data.push_back(0); }
    inline BigInt(const std::string &num) : _data(), _isnegative(false)
    {
        copyFromHexString(num);
        trim();
    }
    inline BigInt(const int32_t n) : _isnegative(false) { copyFromLong(n); }
    inline BigInt(const_data_t data) : _data(data), _isnegative(false) { trim(); }
    inline BigInt &operator=(std::string s)
    {
        _data.clear();
        _isnegative = false;
        copyFromHexString(s);
        trim();
        return *this;
    }
    inline BigInt(const BigInt &a, bool isnegative) : _data(a._data), _isnegative(isnegative) {}
    inline BigInt &operator=(const int32_t n)
    {
        _data.clear();
        copyFromLong(n);
        return *this;
    }

    // ignores negative sign
    inline void toBytes(std::string &out)
    {
        out.clear();
        out.resize(_data.size() / sizeof(char) * sizeof(base_t));
        memcpy(out.data(), _data.data(), _data.size() * sizeof(base_t));
    }
    inline void fromBytes(std::span<const char> bytes)
    {
        const char *byte = bytes.data();
        int res = bytes.size() * sizeof(char) / sizeof(base_t);
        if (bytes.size() % sizeof(base_t))
        {
            res++;
        }
        _data.resize(res);
        memcpy(_data.data(), byte, bytes.size());
    }

    // for pooling optimize
    inline void clear(const uint32_t reset_num)
    {
        _data.clear();
        _data.push_back(reset_num);
    }
    inline void operator=(const BigInt &right)
    {
        _isnegative = right._isnegative;
        _data.resize(right._data.size());
        memcpy(_data.data(), right._data.data(), right._data.size() * sizeof(uint32_t));
    }

    // montgomery multiple
    static BigInt monRedc(const BigInt &x, const mon_domain &md);
    inline static BigInt monMul(const BigInt &a, const BigInt &b, const mon_domain &md) { return monRedc(a * b, md); }
    BigInt toMon(const mon_domain &md)const;
    BigInt fromMon(const mon_domain &md) const;
    static BigInt MontgomeryModularMultiplication(const BigInt &a, const BigInt &b, const BigInt &n, const mon_domain &md);
    inline BigInt modenMon(const BigInt &exp, const BigInt &p, const mon_domain &md) const;

    static BigInt Zero;
    static BigInt One;
    static BigInt Two;

private:
    // abs small than
    inline bool smallThan(const BigInt &b) const
    {
        if (_data.size() == b._data.size())
        {
            for (BigInt::data_t::const_reverse_iterator it = _data.rbegin(), it_b = b._data.rbegin();
                 it != _data.rend(); ++it, ++it_b)
                if ((*it) != (*it_b))
                    return (*it) < (*it_b);
            return false; // equal
        }
        else
            return _data.size() < b._data.size();
    }
    inline bool smallOrEquals(const BigInt &b) const
    {
        if (_data.size() == b._data.size())
        {
            for (BigInt::data_t::const_reverse_iterator it = _data.rbegin(), it_b = b._data.rbegin();
                 it != _data.rend(); ++it, ++it_b)
                if ((*it) != (*it_b))
                    return (*it) < (*it_b);
            return true; // equal
        }
        else
            return _data.size() < b._data.size();
    }
    // abs equal
    inline bool equals(const BigInt &a) const
    {
        return _data == a._data;
    }

    inline BigInt &leftShift(const uint32_t n)
    {
        int k = n >> (BigInt::basebit);      // 5
        int off = n & (BigInt::basebitchar); // 0xFF

        int inc = (off == 0) ? k : 1 + k;
        for (int i = 0; i < inc; ++i)
            _data.push_back(0);

        if (k)
        {
            inc = (off == 0) ? 1 : 2;
            for (int i = _data.size() - inc; i >= k; --i)
                _data[i] = _data[i - k];
            for (int i = 0; i < k; ++i)
                _data[i] = 0;
        }

        if (off)
        {
            BigInt::base_t T = BigInt::base;     // 0xffffffff
            T = T << (BigInt::basebitnum - off); // 32
            // left shift
            BigInt::base_t ch = 0;
            for (std::size_t i = 0; i < _data.size(); ++i)
            {
                BigInt::base_t t = _data[i];
                _data[i] = (t << off) | ch;
                ch = (t & T) >> (BigInt::basebitnum - off); // 32,the highest
            }
        }
        trim();
        return *this;
    }
    inline BigInt &rightShift(const uint32_t n)
    {
        int k = n >> BigInt::basebit;
        int off = n & BigInt::basebitchar;

        if (k)
        {
            for (int i = 0; i < _data.size() - k; ++i)
                _data[i] = _data[i + k];

            _data.resize(_data.size() - k);

            if (_data.size() == 0)
                _data.push_back(0);
        }

        if (off)
        {
            BigInt::base_t T = BigInt::base;
            T = T >> (BigInt::basebitnum - off);

            BigInt::base_t ch = 0;
            for (int i = _data.size() - 1; i >= 0; --i)
            {
                BigInt::base_t t = _data[i];
                _data[i] = (t >> off) | ch;
                ch = (t & T) << (BigInt::basebitnum - off);
            }
        }

        trim();
        return *this;
    }
    inline BigInt &truncate(const uint32_t n)
    {
        int k = n >> BigInt::basebit;
        int off = n & BigInt::basebitchar;

        if (_data.size() > k)
        {
            _data.resize(k + 1);
        }

        if (off)
        {
            BigInt::base_t T = BigInt::base; // 0xFFFFFFFF
            T = T >> (BigInt::basebitnum - off);
            _data[k] = _data[k] & T;
        }
        return *this;
    }
    inline BigInt &add(const BigInt &b)
    {
        if (_isnegative == b._isnegative)
        { // same sign
            // ref
            BigInt::data_t &res = _data;
            int len = b._data.size() - _data.size();

            while ((len--) > 0) // new high carry number
                res.push_back(0);

            int cn = 0; // carry
            for (std::size_t i = 0; i < b._data.size(); ++i)
            {
                BigInt::base_t temp = res[i];
                res[i] = res[i] + b._data[i] + cn;
                cn = temp > res[i] ? 1 : temp > (temp + b._data[i]) ? 1
                                                                    : 0; // 0xFFFFFFFF
            }

            for (std::size_t i = b._data.size(); i < _data.size() && cn != 0; ++i)
            {
                BigInt::base_t temp = res[i];
                res[i] = (res[i] + cn);
                cn = temp > res[i];
            }

            if (cn != 0)
                res.push_back(cn);

            trim();
        }
        else
        { // diff sign
            bool isnegative;
            if (smallThan(b)) // abs < b
                isnegative = b._isnegative;
            else if (equals(b)) // abs == b
                isnegative = false;
            else // abs > b
                isnegative = _isnegative;

            _isnegative = b._isnegative;
            sub(b);
            _isnegative = isnegative;
        }
        return *this;
    }

    inline BigInt &sub(const BigInt &b)
    {
        if (b._isnegative == _isnegative)
        { // same sign

            BigInt::data_t &res = _data;
            if (!(smallThan(b))) // abs > b
            {
                int cn = 0; // borrow
                // bigger subs lesser
                for (std::size_t i = 0; i < b._data.size(); ++i)
                {
                    BigInt::base_t temp = res[i];
                    res[i] = (res[i] - b._data[i] - cn);
                    cn = temp < res[i] ? 1 : temp < b._data[i] ? 1
                                                               : 0;
                }

                for (std::size_t i = b._data.size(); i < _data.size() && cn != 0; ++i)
                {
                    BigInt::base_t temp = res[i];
                    res[i] = res[i] - cn;
                    cn = temp < cn;
                }
                trim();
            }
            else // abs <= b
            {
                _data = (b - (*this))._data;
                _isnegative = !_isnegative;
            }
        }
        else
        { // diff sign
            bool isnegative = _isnegative;
            _isnegative = b._isnegative;
            add(b);
            _isnegative = isnegative;
        }
        return *this;
    }

    inline void copyFromHexString(const std::string &s)
    {
        std::string str(s);
        if (str.length() && str.at(0) == '-')
        {
            if (str.length() > 1)
                _isnegative = true;
            str = str.substr(1);
        }
        int count = (8 - (str.length() % 8)) % 8;
        std::string temp;

        for (int i = 0; i < count; ++i)
            temp.push_back(0);

        str = temp + str;

        for (int i = 0; i < str.length(); i += BigInt::base_char)
        {
            base_t sum = 0;
            for (int j = 0; j < base_char; ++j)
            {
                char ch = str[i + j];

                ch = hex2Uchar(ch);
                sum = ((sum << 4) | (ch));
            }
            _data.push_back(sum);
        }
        reverse(_data.begin(), _data.end());
    }
    inline char hex2Uchar(unsigned char ch)
    {
        static char table[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
        if (isdigit(ch))
            ch -= '0';
        else if (islower(ch))
            ch -= 'a' - 10;
        else if (isupper(ch))
            ch -= 'A' - 10;

        return table[ch];
    }

    inline void copyFromLong(const int32_t n)
    {
        int64_t a = n;
        if (a < 0)
        {
            _isnegative = true;
            a = -a;
        }
        do
        {
            BigInt::base_t ch = (a & (BigInt::base));
            _data.push_back(ch);
            a = a >> (BigInt::basebitnum);
        } while (a);
    }
    inline static void div(const BigInt &a, const BigInt &b, BigInt &result, BigInt &ca)
    {
        // 1.copy a,b
        temp_warpper cb(b);
        ca._isnegative = false;
        ca._data = a._data;

        BigInt::bit bit_b(cb);
        // margin the carries
        while (true) // abs less than
        {
            BigInt::bit bit_a(ca);
            int len = bit_a.size() - bit_b.size();
            temp_warpper temp(0);
            // find carrier
            while (len >= 0)
            {
                temp = cb << len;
                if (temp == 0)
                    std::terminate();
                if (temp->smallOrEquals(ca))
                    break;
                --len;
            }
            if (len < 0)
                break;
            BigInt::base_t n = 0;
            while (temp->smallOrEquals(ca))
            {
                ca.sub(temp);
                ++n;
            }
            temp_warpper kk(n);
            if (len)
                kk->leftShift(len);
            result.add(kk);
        }
        result.trim();
    }

    std::vector<base_t> _data;
    bool _isnegative;
};