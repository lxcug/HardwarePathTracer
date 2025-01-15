// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg(inout uint prev)
{
    uint LCG_A = 1664525u;
    uint LCG_C = 1013904223u;
    prev       = (LCG_A * prev + LCG_C);
    return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint seed)
{
    return (float(lcg(seed)) / float(0x01000000));
}
