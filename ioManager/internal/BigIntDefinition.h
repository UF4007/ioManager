inline io::encrypt::rsa::BigInt io::encrypt::rsa::BigInt::monRedc(const io::encrypt::rsa::BigInt &x, const mon_domain &md)
{
    temp_warpper u = x;
    u->truncate(md.Rbits);
    u = u * md.k;
    u->truncate(md.Rbits);
    // u = ((x % md.R) * md.k) % md.R;
    temp_warpper v = u;
    v = v * md.mod;
    v = v + x;
    v->rightShift(md.Rbits);
    // v = (x + md.mod * u) / md.R;
    if (md.mod <= v)
        v = v - md.mod;
    return v;
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::BigInt::toMon(const mon_domain &md) const { return monMul(*this, md.r, md); }
inline io::encrypt::rsa::BigInt io::encrypt::rsa::BigInt::fromMon(const mon_domain &md) const { return monRedc(*this, md); }
inline io::encrypt::rsa::BigInt io::encrypt::rsa::BigInt::MontgomeryModularMultiplication(const io::encrypt::rsa::BigInt &a, const io::encrypt::rsa::BigInt &b, const io::encrypt::rsa::BigInt &n, const io::encrypt::rsa::mon_domain &md)
{
    temp_warpper am = a, bm = b;
    am = am->toMon(md);
    bm = bm->toMon(md);

    temp_warpper cm = monMul(am, bm, md);

    return cm->fromMon(md);
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::BigInt::modenMon(const io::encrypt::rsa::BigInt &exp, const io::encrypt::rsa::BigInt &p, const io::encrypt::rsa::mon_domain &md) const
{
    BigInt::bit t(exp);

    temp_warpper d(1);
    for (int i = t.size() - 1; i >= 0; --i)
    {
        d = io::encrypt::rsa::BigInt::MontgomeryModularMultiplication(d, d, p, md);
        if (t.at(i))
            d = io::encrypt::rsa::BigInt::MontgomeryModularMultiplication(d, *this, p, md);
    }
    return d;
}