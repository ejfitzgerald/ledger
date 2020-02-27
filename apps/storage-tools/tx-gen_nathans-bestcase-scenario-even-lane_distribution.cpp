//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "chain/transaction_builder.hpp"
#include "chain/transaction_serializer.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/digest.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/ecdsa.hpp"
#include "vectorise/threading/pool.hpp"
#include "storage/resource_mapper.hpp"
#include "crypto/identity.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using fetch::byte_array::ConstByteArray;
using fetch::crypto::ECDSASigner;
using fetch::chain::TransactionBuilder;
using fetch::chain::TransactionSerializer;
using fetch::chain::Address;
using fetch::threading::Pool;
using fetch::serializers::LargeObjectSerializeHelper;

using SignerPtr  = std::unique_ptr<ECDSASigner>;
using AddressPtr = std::unique_ptr<Address>;

static ConstByteArray all_private[256] = {
ConstByteArray{"VuTMmLs2VopMl4JP0Kor4n/MqxDkqkXkcR+fB9AgfxA="}.FromBase64(),
ConstByteArray{"BDkMTozbRd00cES1bFh7kSWdyR+7fewbTd2xPbZ3cI0="}.FromBase64(),
ConstByteArray{"utFbTH/KFECngjp5s1i4aZvZBbJbmLflHRfvmhmuvBc="}.FromBase64(),
ConstByteArray{"6j8YPk28VhWbPlGetFF1cvKSfvL0+JvqqGpww22CSBI="}.FromBase64(),
ConstByteArray{"aKsteJSTCe5wO4Y18dAQq8o2oXp7CIYkkndGNJJJPhg="}.FromBase64(),
ConstByteArray{"h56wCfDHgKt3x0kEcbXSMS2L0eTB6VG60O07Kc1eObg="}.FromBase64(),
ConstByteArray{"6sTrwDaCYnpwa2NhA19bYLy08QCIFeufZ3aQ29ehP0A="}.FromBase64(),
ConstByteArray{"Y+oLBZglV8XYrycybQrzUEcyG/1QhbYS5aXrcqRbDLU="}.FromBase64(),
ConstByteArray{"TxVcTz33j115wmONRyk74VcvvgkwlIheTEXAgMNQUss="}.FromBase64(),
ConstByteArray{"Wdv6HZ7iFzcQ92ByBP/3OmiEQQxY1oj2roXfoxwBesA="}.FromBase64(),
ConstByteArray{"os2oqVcd67RrcSXdZFfOBJQaKhtQf/P8nP/KRksfIb4="}.FromBase64(),
ConstByteArray{"1W/bzIf/giN9qgmBCBu5gr76aYNynIxafipnv0N46YU="}.FromBase64(),
ConstByteArray{"VcEWZg11SUHuUsjsf4VKfeubgmsiKTpZWOQ21ByORFM="}.FromBase64(),
ConstByteArray{"TlRs6I7kk64rj7sWddAchqCgAQNXHaqAz0lnE3Fboe4="}.FromBase64(),
ConstByteArray{"Ks8wSBgv76U9LRkS78lTVGTVYOyAxP23UNJB43lGYfE="}.FromBase64(),
ConstByteArray{"FpT2epTK0KvpiWPlFhqu1T00Z2jTjVP2UhyXFEIWffU="}.FromBase64(),
ConstByteArray{"ohvyiw438DPLIo33d/pZVin2GPSyKOty56L4uQfrpfE="}.FromBase64(),
ConstByteArray{"1VCyLG4vpqM5KV0EY5ZwJEsCHJbY06E8MtNS5bD1gQo="}.FromBase64(),
ConstByteArray{"AUe+iOFGkUmQd5qd+efZtr2nn96mXisAhEZarWXbsFE="}.FromBase64(),
ConstByteArray{"M2pU7igGTLvK5ZV6YkzeMEsSkYn2wMa2+WPrPqyLIBM="}.FromBase64(),
ConstByteArray{"qa4akLEpKvL46RJDxMoNlZn4XAmIq02tSvCw8OHRXfg="}.FromBase64(),
ConstByteArray{"Bv7JVi4EOsrs5YUT9IZAHGUqTreTaAK9526WUx4WioY="}.FromBase64(),
ConstByteArray{"7Vhllon+EK5tZNnRn2zxVWpjh0IzZHVKkwd+bCEiPAk="}.FromBase64(),
ConstByteArray{"8K9eLYvI0uMC2Y/2lj74hOVoS7G3J9tiTFSJeLDhzu4="}.FromBase64(),
ConstByteArray{"PXPV5cz3zHlsYQy93SpFGcr3H8ojdokAONj/vit5uL8="}.FromBase64(),
ConstByteArray{"l3WjHuEWP2gE0NQjcCh++Hsp0ICy5CQFw6z4uO8mIu4="}.FromBase64(),
ConstByteArray{"iCkAAlO5fmwhaoFVleYKD32erhmia5ntS1rfUBrZx4k="}.FromBase64(),
ConstByteArray{"L01QKoLiSEyzS8eXFOVUNREvXdQ3DELKBcHXinWmqow="}.FromBase64(),
ConstByteArray{"vF8sBVzASK5q6B7nXS55mflWoya1E6eIqQoZ/Nzxz90="}.FromBase64(),
ConstByteArray{"FmR99hPGqh1Ljz28ogZ/3zbZacxHy6Y6LnJQ69m6URQ="}.FromBase64(),
ConstByteArray{"J8/ABCWtKaOinp5vAr7GESzT9tbPFMVKD04IiZCPEXw="}.FromBase64(),
ConstByteArray{"n8Xqw4/pcq7v/Mu/ZPn9Zh1YoTYSlTCZhTpeyxUeaec="}.FromBase64(),
ConstByteArray{"8YEL7alRHrRDc2ivE8fMrq/hhhDtUI246z+8n0oxYrE="}.FromBase64(),
ConstByteArray{"2jn/l7w+mCFzrVTuI/b4NEHP7aEAp4Bh4Q8rPfX/Dvg="}.FromBase64(),
ConstByteArray{"9QoXt+ZnyQ9VqemAvYX9V7RPYBmweDQZgn8i2Cv4EOc="}.FromBase64(),
ConstByteArray{"LBV6hoLMy2rFmG3g7VXZzZvDaEB3WKcI4lpqEK9irWg="}.FromBase64(),
ConstByteArray{"fddE3cCp4NOrcD7o12rXauz7tNEqY/tBIIqFgTE2Gg4="}.FromBase64(),
ConstByteArray{"o5UcfbzepMUMKG0XdYndCVT54BdJSOlTn/fliBMMWgs="}.FromBase64(),
ConstByteArray{"98jAcL93XOREuKUxSsl9prJ7prgLxjk1L4NYBrxMFJ8="}.FromBase64(),
ConstByteArray{"PD4gXaTNkx8N5e6IFPX0Tl2sexdvtj8ELiG5+2OglNY="}.FromBase64(),
ConstByteArray{"hY5CwAojD15PleXEcW+wNTgGg+Hyhn58I0722YJmRng="}.FromBase64(),
ConstByteArray{"nsYZG9JU4NX1YHsoUUN4PxvvKr3ZbIJFGqZcQO7pmtw="}.FromBase64(),
ConstByteArray{"Yhd630DgR9WY6wfil7/SaOp/enF6Mip1f0xv1ik24IE="}.FromBase64(),
ConstByteArray{"gI2hQMA47Rkk1WDUVQdzuHoZJCgcChkz7keUQi8QX4I="}.FromBase64(),
ConstByteArray{"w/ECZPtPu9aKwa/qS1QnWi/T1VzjxI3F4zXvEqe1ml4="}.FromBase64(),
ConstByteArray{"tBkVGAbGdVq1d5mwlqqXHv8vNDAGHI0uGEH8PEsGEw=="}.FromBase64(),
ConstByteArray{"IRSzuEcd9qtksTE4lffpv8scIAljDDZsKuIWygl6yH8="}.FromBase64(),
ConstByteArray{"FKZnFmwufRrBeoT7NZqK8ZpzRzyfX3NpcY7dhOEbZ+A="}.FromBase64(),
ConstByteArray{"Off1UDbXPzqlJRsDSlq3rBV7z5G7nFD94FigDSISEoM="}.FromBase64(),
ConstByteArray{"fX9J+txv1ok3gLrN4jTxDkaJdJrLhieMZJGMA9mGX9c="}.FromBase64(),
ConstByteArray{"GJCmqZ6QwYAiFi4v1Lr4UohomWSJzOLv29cAPnGDbZw="}.FromBase64(),
ConstByteArray{"YMTfdU0GOYn5BM8uu+WSCghAX6FN0wVPTmCteH90OYU="}.FromBase64(),
ConstByteArray{"PSeUGgVHY1pB32DITZBJWlfyt2+1ikkT8z1HBzVZ0Q=="}.FromBase64(),
ConstByteArray{"dAFTFQO3CeMJhrabOTuegkEDhFrpekYdLQtnw360rsg="}.FromBase64(),
ConstByteArray{"0FiJeqQ4NZGOI4iUFuotllhFWaP5biC6Y59w9GUW0QY="}.FromBase64(),
ConstByteArray{"MhQAra8xIjMPYymPpzt5775izvyDzLHqS2WQnxTKPQ8="}.FromBase64(),
ConstByteArray{"21r3lZL4Gi2uqBgL8ZvrDcaJxL5MR3wjWZL2mpn9F5o="}.FromBase64(),
ConstByteArray{"qvKcsSUzKN3Mq3o/1EgNIfCbE2ACD0il/5YKvzvUbwg="}.FromBase64(),
ConstByteArray{"T828psapZ/jxwVfIOMEf0jaN+bj85+LwueDqUJ6/2vI="}.FromBase64(),
ConstByteArray{"eoIUPjXnTCOIbundYPAsPohvGX/DA7Ju68MwFJzxyoQ="}.FromBase64(),
ConstByteArray{"7/bpTL+Edx52k3OqdD6t5t0oru7jX9G41LeRbO1ILcg="}.FromBase64(),
ConstByteArray{"m7G/eiKc80kJldaniruvojH0+mqZAE1c1Kg/hUGD3RQ="}.FromBase64(),
ConstByteArray{"jQfX+yXFVsuCHrFk2yQUJ30akcSW0EEMNuluf8kRSPU="}.FromBase64(),
ConstByteArray{"d+JkrMw11qXp0dkDvESk33U2wvhNW8KrtepEo/M74zo="}.FromBase64(),
ConstByteArray{"+GsZvWsBLNoBO61BUVhBkhFCYM4v3/zyxhHuzpyVlz4="}.FromBase64(),
ConstByteArray{"78grCJwYEsXocAQAqsqlE4rIfK3Frv4KbuG40ZxaQfw="}.FromBase64(),
ConstByteArray{"Om5wYYeU/jt8qiNMoGTztzJ/tFJ9FSHbJuAcQO2ipEk="}.FromBase64(),
ConstByteArray{"OGmbBasffdHvNKBsTEig4akeVC4kMiAhXAUvTmOFt6A="}.FromBase64(),
ConstByteArray{"jljy61DQZwcb4vUN3JI6Rw4kO+DnSogRGjd3DxBKu/8="}.FromBase64(),
ConstByteArray{"W8NDeEXqG5y09yEw1UMtEFWADU2FDU1AXq9dt6aslsg="}.FromBase64(),
ConstByteArray{"/i4x31ES3IRxNCXPzAdPhy1v1jnYKUJmPTq0BsZY6U0="}.FromBase64(),
ConstByteArray{"zJ7Z6jgDatK5S4HLpMnnsPqX/FNMSEW2nG+27UiAY2g="}.FromBase64(),
ConstByteArray{"VUltISVXG1a5P3Im12BYK/J0Fu2peX8+vcBRZySLW5g="}.FromBase64(),
ConstByteArray{"7I/V++7Ha1H0n+mrN4mmyyz7NiFoKcwzCr8kyprlPhk="}.FromBase64(),
ConstByteArray{"NrmnTOmfezsmjb2zhBI9B3M8uCaU3IoOPhsD+RHRfwg="}.FromBase64(),
ConstByteArray{"VQdhgZzxtIfjROWhQw+P+LKuPStjeHi0wtyufZzpNhc="}.FromBase64(),
ConstByteArray{"K+qqiXtz8tOndV3U7+5noL4j2Li4C9Mg3FaiPvSkf4Q="}.FromBase64(),
ConstByteArray{"vz4ORPZFdqKWM/Yk6wJvS9mppU6RofEo0+wftFXVRts="}.FromBase64(),
ConstByteArray{"NAhY6TYwPm0e1+1wLAnnGurfAtav0FsdKDlSGwLujds="}.FromBase64(),
ConstByteArray{"le8g9hmFa/IJfnSCUPRYRgyrAwdQZqHPau7TZ4ZFfdg="}.FromBase64(),
ConstByteArray{"JTxnr9sZ8C0L8Mi6sa1FTmuuH8uuq6f2s5yyYwHl52Q="}.FromBase64(),
ConstByteArray{"4fl3d6Km5eBgoKcoI7SjB3fDJNnUMrN6PRlyHGGrS4w="}.FromBase64(),
ConstByteArray{"R02OkMTRrL//9XmX213UDqFoObCqSOgyZgqCutCVqiM="}.FromBase64(),
ConstByteArray{"1sRTk0VM5ljYvsSzSEcy2Cp59jR9ukoR8KjNjM9QuOQ="}.FromBase64(),
ConstByteArray{"vBsZOmvy14K/fSeSjfR4+yTF0es7q8pes5pmVE8vaeA="}.FromBase64(),
ConstByteArray{"vZSFQpK8unzj5TpFabegxazzmaQJAtbKXaOQh4G8vow="}.FromBase64(),
ConstByteArray{"S/9g1CfGMgvoYxIu2lB+OeWTcG/byuO4C24qWB9ZpTY="}.FromBase64(),
ConstByteArray{"jy4ylGpKlj44e6Y5wx3vMtxOPFf7R9CQGgD1gJ7lUmI="}.FromBase64(),
ConstByteArray{"T3XdQF86gabXFyH3u9iBSy2Yora74TiF7rDznggEd90="}.FromBase64(),
ConstByteArray{"i0cAm5pPbADt4SnFYRI5AdiCByx2BNcmbEZbU9u8Ovs="}.FromBase64(),
ConstByteArray{"oS8fVLhOCZ8/DZ/5dvpBhQsf746KkGoJdr91rUGTico="}.FromBase64(),
ConstByteArray{"+rGY/23aqx+99/U9zMviLh0y4bJqda0TqAoT7x9qXp4="}.FromBase64(),
ConstByteArray{"naCmoALajQg5Bt16Drijo75SqS2I2KgtZyCa9cbwhrM="}.FromBase64(),
ConstByteArray{"659Er3C57a7wFWMsI1DTnjSv0YX+0AFjqcGbNUhxMOE="}.FromBase64(),
ConstByteArray{"SEwE0kPSNgvD+hRnRSmnhHAIlvG9wHknjaqtQovyt28="}.FromBase64(),
ConstByteArray{"MJvARuKvJeDuMBVfElr5UoxRH+5R/wvIqJiOEi71KVs="}.FromBase64(),
ConstByteArray{"lf3N99erXA8xRiPQTDkHGNOF/5GstlZ0CP7x5/sGyJQ="}.FromBase64(),
ConstByteArray{"PslwbdbkeR3zwpuefkU2SdyhLaN2Iy0r+T9YohHbegQ="}.FromBase64(),
ConstByteArray{"Wn4iSMjMpvFVj8mNZoYMgb/aLGKnPcTZ35i0hwoXErU="}.FromBase64(),
ConstByteArray{"Hh2OJuu/pDTe9EGPvg/mB0WaOZp/wl8nrzQqyHBxyvs="}.FromBase64(),
ConstByteArray{"KXRerj7EcAzdzLE3GeK53pHd3SAtXuY72jVuiRnbJ0Y="}.FromBase64(),
ConstByteArray{"lcDAPkGVDPFTrIYGL2P5fNrGx3czdtHbjPZi0ByfgZo="}.FromBase64(),
ConstByteArray{"+yBniKmjz3OIAR26dAB6z2p1PL2ECgye9RvQvdrxBEw="}.FromBase64(),
ConstByteArray{"JGC/Z4wIaG6Oq9XK7D6YLV/0u1rXj3/V7b/hzcf5ZrM="}.FromBase64(),
ConstByteArray{"HghmR4SsrXnbcmQg8Kc6dUnSCdAuO7AKJrMiSoGB4xg="}.FromBase64(),
ConstByteArray{"uXvZCHh7FGHPHCVYqSSGJUq4B3FDAiGzeaey4Xnfehc="}.FromBase64(),
ConstByteArray{"JubuHlm/m7DhlvECu6plZ0o0a/ueRsenbaMoBuFHH/U="}.FromBase64(),
ConstByteArray{"/er6Ik6agOXcOuJ9yDGZVuG9IETzhrsM2m8wTZNCYG8="}.FromBase64(),
ConstByteArray{"t9VeuG+pchgFR/ftCrW3Y2QP3zSKmJRql0XESmcFL1Q="}.FromBase64(),
ConstByteArray{"dOwc9V26dBJl2LF8kTISzGAnUH74LL1T2im0DO+8Rng="}.FromBase64(),
ConstByteArray{"yNdTPLMauu6sreOVUkBCGto1VCHHGUcxBwxZrcv5dc4="}.FromBase64(),
ConstByteArray{"YUTcvM1LLEpf3CN3MQiPF28zQ01IIKB4XQ+Ej+QURTs="}.FromBase64(),
ConstByteArray{"/l6MsACUHlAOxcpWsF9eCa6UpDUVPdgDNJXtoWvULw=="}.FromBase64(),
ConstByteArray{"Giu8JC9d7v2hE8JC1h2LCR4SXpf6DFvkf/5qqd43cXQ="}.FromBase64(),
ConstByteArray{"zAvQnMWrgRho1CLeNQx/xRGUwHBNXXP+eVC18Sy4dUg="}.FromBase64(),
ConstByteArray{"NMyLJvs2AVrxa6edlr6cCjRxRdUUIy7R4Abc0Ls2VPU="}.FromBase64(),
ConstByteArray{"pxXSje3YjEdpytDZmt1ANSzeMKFgqqBLPoyp1bzHBXQ="}.FromBase64(),
ConstByteArray{"2kxM+KWzDTHnK6dQ1YogCFlbOOQ95kVsBwwOMvq8mds="}.FromBase64(),
ConstByteArray{"Uj8RhywQhyaHpwbFhg/sKnablMaFvIsSdGr4+aDja9c="}.FromBase64(),
ConstByteArray{"MH32A4Tm/X/1Tpr7rPJShETtT+W0FwGARXnsQvrLyFg="}.FromBase64(),
ConstByteArray{"kfB6BZzvvP3U1QCjhuYKubMabld3k5L8yaS8CodrYDI="}.FromBase64(),
ConstByteArray{"ne/3DL2Q1SND9davwViOFKwwo/YtwGjReihwXEaE4ys="}.FromBase64(),
ConstByteArray{"cnlZNAftAYHoJgWONkhYsjP0CaxR9MFBGNvVLrxRtHk="}.FromBase64(),
ConstByteArray{"BYHW+5XETCDEaYUeD5/MYrVxAjJRSXQNViKLR7gHK0E="}.FromBase64(),
ConstByteArray{"qX6jwb6FjyjfXqP3hbQab+kyFBhhStHImeXmf9fIK6A="}.FromBase64(),
ConstByteArray{"WH5wKp0iuGyDo9qVlfS4SXr3XUs09ILGZwcmYSGg9r4="}.FromBase64(),
ConstByteArray{"J/1/O8NpLDYcCeH7CWoc8wl+lveZZqDkhKi5OslO4yI="}.FromBase64(),
ConstByteArray{"LDVa1Nyymj7CKR2vB7NABnXqXAICZTZ7UxxZJzU4uEI="}.FromBase64(),
ConstByteArray{"qJGsobHUFXXodact/gc6ZRXH/cHoe4ocDmaqEWDsUmE="}.FromBase64(),
ConstByteArray{"nmLg1oQIIBYN2obhyqbpxc4qoCXkrF0tPyBDDrftey4="}.FromBase64(),
ConstByteArray{"EcRew6HDmq09zt6ySs+Fw9MA9Z82ewYvjnq2eztCxwQ="}.FromBase64(),
ConstByteArray{"E6OtbRzYAYm2O080+NkyCn8ApNWhO96QBthm3AP6p9g="}.FromBase64(),
ConstByteArray{"kgyINdYHbDtTEOaCQldiYbhWs+vXa/iW20GZtD524TE="}.FromBase64(),
ConstByteArray{"zeZFRYqcstYXke9jux3HDI/WSVErf8Rw7dAmPghFD2s="}.FromBase64(),
ConstByteArray{"0029s3nLeWQEwoMJOKLng/QlKpooAPoJuPh/U2UZk0o="}.FromBase64(),
ConstByteArray{"Ek8AJfqRDKUgs6tf+cdG4enb6MyW9UPmG75PGMcNGe0="}.FromBase64(),
ConstByteArray{"L+R8nKlhEgWhm0ts/peKiOXaLLmVLMglVfs8oY5oXbI="}.FromBase64(),
ConstByteArray{"3g3YtEE0DV1Cmu2ik+chBRf6FYvxuxaIuSVSVpPBxPw="}.FromBase64(),
ConstByteArray{"f5uOTvq1HopWAe44i01gELx5KTsdlxnwE+tpoV8gPqA="}.FromBase64(),
ConstByteArray{"D4rJCJLmkNa4Hiyr9teY8pebcarDr5lCveCqTvumG2c="}.FromBase64(),
ConstByteArray{"d/qI2P26EZAxrxCWjcR++SMM6Gf0d7vCeQe8kAcfvbc="}.FromBase64(),
ConstByteArray{"nykGSSgSyOP2u+vnD39uw7gP9PxK+ddJuA59o17eCuc="}.FromBase64(),
ConstByteArray{"iEYagNrFpmGfQ+Ha0ZICCI+bXVj2ZmfSYTaiokTT72E="}.FromBase64(),
ConstByteArray{"SzP4N43BMAIN94o7ZVURUQX5kV9wcm4Tu2sZFAe4K2A="}.FromBase64(),
ConstByteArray{"36k7MCD3dLppoNMNffblQbIWwcGbWyJypmi4XdX2j/8="}.FromBase64(),
ConstByteArray{"9H5A5kRs0vIpYd0X05CuCuz+21S/2FwxSKjrod4UfgY="}.FromBase64(),
ConstByteArray{"d47OXgxwf+h2o3fHRADXKKa/D+LAbvfE79l+oI9KZWo="}.FromBase64(),
ConstByteArray{"I1tCnaXog5FlRuvobNqCJR3x4cuBE/dqHjoU8wGy0xs="}.FromBase64(),
ConstByteArray{"JiMiIAlg9OQo+RiYxeoNhEopusO/syTGLqgsbtGYN58="}.FromBase64(),
ConstByteArray{"pRKoB5inf2CsBFPs7fTs5TZumJb+96KpG1o68ElL9KU="}.FromBase64(),
ConstByteArray{"kTuai9IAFPEiI4nSyO7PdI41AIHZSglge40VtixNLGI="}.FromBase64(),
ConstByteArray{"cEBEdtV7qMoWuKz1BC77RoiQo1E1+e1+yOp5sZ3y70o="}.FromBase64(),
ConstByteArray{"ywy6vhwd8FmKuF5GYzPMlvAnfOrMszi8OLOzwAjO2q0="}.FromBase64(),
ConstByteArray{"EBaBrS0R4OIcN8UpEF91eZEeILkYSUmvaSiucqT8N5k="}.FromBase64(),
ConstByteArray{"cFGSF1Dx7fhU5aUQSJoCpQbHfYr66C0WWQQ7Q7U3agU="}.FromBase64(),
ConstByteArray{"/h5pVhwBhEUALeuODzHEX+FqAD9KF4kjFpmiMdcNf38="}.FromBase64(),
ConstByteArray{"FcDLJoeRH1608U8TUqbcTSP8/RVWa6js8Y4byPCGGGk="}.FromBase64(),
ConstByteArray{"PjNgQCAiFrO1zydn/z/RopfwYzIo+XGbDP8olDY1WL0="}.FromBase64(),
ConstByteArray{"ioyyds58c/X7be/nQD350UF/MyDs5sLJz9tW2XBifl8="}.FromBase64(),
ConstByteArray{"EhDc5fq9Gc1cE9oTvFl2exdUIrfunMpch8KLZJwjoU4="}.FromBase64(),
ConstByteArray{"Yc7qSv4deH4hrjyjhbZ9Iq3OUqGwSAzSo/X5YNGNZzw="}.FromBase64(),
ConstByteArray{"7ArPcnTi9VrixA3lJ4+LL8qL7CsHD3MmofdLm9vI4yM="}.FromBase64(),
ConstByteArray{"e0wPhDwUvIRnD/i76e1oYAM0IQETiX5WhkzrLBqi/uE="}.FromBase64(),
ConstByteArray{"7kE7zjL+YnU4V4RdDMISLtxjRqZpPjLIEjZCzSV3DjI="}.FromBase64(),
ConstByteArray{"3i+W+7bBTFUiwmVWGPDxaID7QG3pvTrDxxkqu9O/qQg="}.FromBase64(),
ConstByteArray{"Z533c9rKS1IzQXePm74udGh+UCwjYG/W/ePiBoTa/O8="}.FromBase64(),
ConstByteArray{"7X1gxiaSOKH7ks3JfBEwPkCFQS+bMuLjWKnZKOiLqqw="}.FromBase64(),
ConstByteArray{"EfooavTDSwmU8xExLoxtcOw0/MtdEudYJAvoLcjtxLg="}.FromBase64(),
ConstByteArray{"I0iOS0qB9w1fWKxgCuq8v4atttqaYX04cHG9efkgBXI="}.FromBase64(),
ConstByteArray{"XaVTHZdM/OnbatzHYaIDuipxLD2pF++/feGX5b/te/w="}.FromBase64(),
ConstByteArray{"Gm6NtwDkF1PVHbsaPHF/fxLLlBbrHgNj026DbfVUNtY="}.FromBase64(),
ConstByteArray{"roUUUceceAhsO0a3su+EgTBGEFaZNt+ptaFXw6HjNys="}.FromBase64(),
ConstByteArray{"HMSOGNnh2nn2qeUkgxoBUqqrZJXaYg2EY6HlTYjRn3k="}.FromBase64(),
ConstByteArray{"kxhhrY2dOQpdHdSwtPj+S2s1bTfIm1dAnpmwWv1jMas="}.FromBase64(),
ConstByteArray{"3RxLRpnJIyyVvNO7wydQRPe24Y725JnrHWMlufs2RhY="}.FromBase64(),
ConstByteArray{"wl8CqB4AlnQo+jAbSjUrq4m5OsTSALclapqFGdRYw/A="}.FromBase64(),
ConstByteArray{"sNWVdROOu8eX9XgvkkHxddaHow9G4I++uYKNhRKi+ww="}.FromBase64(),
ConstByteArray{"ngopTbiiIeKkSkLYoq7O4dcJJZp/OUm6TAXAMv0tT88="}.FromBase64(),
ConstByteArray{"MOglzr5Fjl+yWq28oEBt3OgdXx6uUk/7XL1aidiFQqo="}.FromBase64(),
ConstByteArray{"H6EvFmJQgrAYomnocTNs6d039XaP+WnC/Lk8SIEJ/Ts="}.FromBase64(),
ConstByteArray{"n/gx9s5+IOSFYCJweeEgM17tUi1mU9q3xGePrmYPx1A="}.FromBase64(),
ConstByteArray{"0pBkD72ce8TwRbnCYOtkEa5Ag+P62PdBaKp5i6DLiOg="}.FromBase64(),
ConstByteArray{"rRAaE/xiyUE7IMBtziqWdmN71A5oMfXCmJ2qhuc9wlw="}.FromBase64(),
ConstByteArray{"Ygjwpkgj6smWkO7p0wqaG9mgbx13WF/tnzk37WPT45I="}.FromBase64(),
ConstByteArray{"7Lj8xvkBFlxEDX4Ps3j89akNmHKezrWS6rKPp0eTn4A="}.FromBase64(),
ConstByteArray{"aQKIo33cux2LEWXQCptXMSl2Bkd0euYGjTSQfJTqMpc="}.FromBase64(),
ConstByteArray{"1AFZ7zi09jw+Tyw4gJh8DllEubr9j6NpBoxpettHbNA="}.FromBase64(),
ConstByteArray{"jx9dT6AhqwNhpXd/LvD29tWUXVEvfHlcroO+VEi1Vzg="}.FromBase64(),
ConstByteArray{"PUaEki1UWYdfcMOtTujSjE/a7lCxyCGTnA1Q85GAw94="}.FromBase64(),
ConstByteArray{"591ECRgNytEPsG61e+z0L2f8lMdfYLfzQ49xg6IXpas="}.FromBase64(),
ConstByteArray{"pgMZ/6qvHqFZONuNTXXPa3Bwbva7wZhpm1oKyFqhjHM="}.FromBase64(),
ConstByteArray{"ikbavaQXjWSFcuAy4nsZUeVlq+iF2w/9Tvp62iWxX4c="}.FromBase64(),
ConstByteArray{"f/eYcuQYKJrxKLfR6oNno8xhK5SDhAiyS60FCM90tok="}.FromBase64(),
ConstByteArray{"zpdEoKhnBteuCay/h4/ZSPYJAGgyTtD/19O8UCLIf+w="}.FromBase64(),
ConstByteArray{"zO8oqzo7tuXhuwMnW8BBoEnMdNXlFqHOldZksKUPiJk="}.FromBase64(),
ConstByteArray{"9hNgEaly+rJWIfPNPKrEy/JccL+/U+gLkfJTfKuhWYQ="}.FromBase64(),
ConstByteArray{"13mRygF7zfXnAAq4JKMwRbsPVE73B8hPh9Tt9dk7+IE="}.FromBase64(),
ConstByteArray{"ISxA/YmB9KKgeR2+oh/gKmx2CyRaanU0uKtV2Nmt+cg="}.FromBase64(),
ConstByteArray{"q4TFA7n/PcB8I3RHH3rfSjkb1LMWI6mzDf0h+Z4s9vw="}.FromBase64(),
ConstByteArray{"ghBsZd+5541rKagmZBOCTfXxC7Lt5ILHopjQJ6Zr4Po="}.FromBase64(),
ConstByteArray{"/o+0RGccM1zIKqiwp3Dv3cXKzEZvsYFXXqh/obhqWlw="}.FromBase64(),
ConstByteArray{"xsG1ZvPx7VW7vpPl7LApcRO/GM5aD1VNzZ6oSjg92wo="}.FromBase64(),
ConstByteArray{"eACHX/vIUHZqbfHOL476NhumvDebhuv41VevGO8L2iw="}.FromBase64(),
ConstByteArray{"1dXdr2lXv+GQpIl9AWK8qES3xJGkNVg03PbMbKXODPw="}.FromBase64(),
ConstByteArray{"TevHpXFAxBeFQyH29NRUqJK6MqSG4HGEHJsU4LVVJl0="}.FromBase64(),
ConstByteArray{"Uovp17XAAbvpq72jOlg41glYBc+qVBJeQMVTJ0Y+qxA="}.FromBase64(),
ConstByteArray{"8cgd5BeG4ieULvNQQpsj9+IEETkDHc1mq0VEkqN0aWo="}.FromBase64(),
ConstByteArray{"wCW9IJCUYeA+VxR+y00n1b2ePkwhtSCdS04Myf8hN1Q="}.FromBase64(),
ConstByteArray{"hkvLhVxpANEVCywHORMrlKNfutFbbv6dKo+kCkIGTYw="}.FromBase64(),
ConstByteArray{"s+ltLyiW0rcibi40Pf8cRhfH23ssWgXE2vjYKWD6SyA="}.FromBase64(),
ConstByteArray{"fawki/YuSGFcne52m3//S5OIyiCPGcLWY12FBAL2Fz4="}.FromBase64(),
ConstByteArray{"H8toW00p+wm9EiikXghyzmiSGJDuNWS4V29sQN1FZLc="}.FromBase64(),
ConstByteArray{"0u3m3juWdWEkpqCqH2jOY3k4IELJPYmVLdpLSjrIOrI="}.FromBase64(),
ConstByteArray{"17fpOB9K+SUgBmcev5ETnbSjUiUemZmwgUgFuVtYgSc="}.FromBase64(),
ConstByteArray{"Zxx3ufKHfxWlnjj/QLRJnOXZejtXF+P0FtRQPabe9qU="}.FromBase64(),
ConstByteArray{"P34ym3xnn89vZX6vKFWAiRhGgJJ17t/PzyQpijyL1os="}.FromBase64(),
ConstByteArray{"uHXfgeok71XLVMtdIoTf3+JIy0F7JQO2Jx944soMoQs="}.FromBase64(),
ConstByteArray{"hDOWBKVyY16opGsNFU9x3ASsuL0dOtb2Z1wxHx6Puro="}.FromBase64(),
ConstByteArray{"tIhItRFHYeBf4nlRXmB7TMi608I+WiAeL+w4MJnB6Ac="}.FromBase64(),
ConstByteArray{"R28U7YYMsf1Hv+kUqa1qv2TS6bJymyeo8DGyI5/+1kM="}.FromBase64(),
ConstByteArray{"5deT3oOGeAXgX94ZnZiCpvcdc8DbQs1AGUUK3FafWLs="}.FromBase64(),
ConstByteArray{"eqlzIo6mjFggayz6bWYchbsvkYbZGzfgi6A4JbOrAmI="}.FromBase64(),
ConstByteArray{"W5jDZ8n/eb7dbCjtbYoNy8fIC1myp0+bbI4SVGDO5G4="}.FromBase64(),
ConstByteArray{"kGzYXxi5InyjkWEkB8Rq0roUYnc57CsvytKzCYGDGG0="}.FromBase64(),
ConstByteArray{"qFJHI5vfjPrcYjHsZC3w+G6HNAJXg+/uwCE9JGCCH+4="}.FromBase64(),
ConstByteArray{"omokvyrAonxb/2hELm1NYxAIPfKI7gqtZgCL3Adz7E0="}.FromBase64(),
ConstByteArray{"dyvP105Y7XI4oaFq61X6K5M1NAsE6j31C4BISBLUCds="}.FromBase64(),
ConstByteArray{"6lCd2QWTCWfyjidqGjADnzRpmVcUIE6KWPPx5bHqlf0="}.FromBase64(),
ConstByteArray{"dFgsBpWy6Wlm6G8CDHUXvz3AvBh0R4eo9WqG01b35GE="}.FromBase64(),
ConstByteArray{"Dsi/VbFfVjmU/vZesOSK5rGTYLLCP9eYWNIws4GdOXI="}.FromBase64(),
ConstByteArray{"1+XPRla0g0R1oKKUzS4G7tnR2ccFTBgj5imSbWtC1oE="}.FromBase64(),
ConstByteArray{"Y/2PImALNzy5ZCwjhorzJS+lpQXzfOu+fgKfp9ZJgDE="}.FromBase64(),
ConstByteArray{"MhdFD4HgR/AZyYiud6iYBW1n3DWFx7Gpg3rTNJIaxA4="}.FromBase64(),
ConstByteArray{"vGQgxRC9ED/rQqbGQ0jyQuAq1z3oAoz6xZ/iwqJxcDA="}.FromBase64(),
ConstByteArray{"5fEOW/29tr5u6oQnNfjE8DpIBZXQMEFmzH/oD6nePpw="}.FromBase64(),
ConstByteArray{"h8HkYrwTtHr0bTM8e0tUNy14LG3cZkVwY5fhd+uPA8s="}.FromBase64(),
ConstByteArray{"XxzRnY8jg07Wgikf6x2oN87uSy/2KGGxospEeioLUxs="}.FromBase64(),
ConstByteArray{"Kb4Wtv/CD51Avr9c9+FtoAGi5iX0StGeKe4rJ/V8HUs="}.FromBase64(),
ConstByteArray{"Js8195bbeVFh7FjkpuVLB9cRRDYyYkFBrK515C5gc48="}.FromBase64(),
ConstByteArray{"djJfOLwnd561ey3J6jEpbWuTosc3uDJmSCML8d+6dkg="}.FromBase64(),
ConstByteArray{"6KKn9xxCmZlYYRTaav+dS0XVzMH6l4mJKLh8PgzW58Y="}.FromBase64(),
ConstByteArray{"lR1WP70WLwHfYQjRMJPQ+ZN8MqWvbxti/aOwg2yyWW0="}.FromBase64(),
ConstByteArray{"+a6OUA3kyOGBbEfqMqjtg2dlGUERedpDMkDa+g0F4Cg="}.FromBase64(),
ConstByteArray{"SctkhbLf36Ig1PpuCQsK6w8trWKkEXIFgoIyD0XwbDQ="}.FromBase64(),
ConstByteArray{"yiwJ35j65DHU+7hhU5sMTd37Kcp1jUiRLrcuFA13hik="}.FromBase64(),
ConstByteArray{"UKtU9/7oCZNYmDd7lCd0LwZxgAnkuanBPqR1NKq05AA="}.FromBase64(),
ConstByteArray{"ZDu6ILKzxb8dhpjgQGnoxMG2xP/MA2MhH177WqEdVRo="}.FromBase64(),
ConstByteArray{"kJCOci6fD/Aa/lXbEN47hIaL0lyRQkVKfDa333HAsvg="}.FromBase64(),
ConstByteArray{"lJj0cLsf9KsOSJHdfW45xV7wNDnGWgJYeeNzDrEMm2M="}.FromBase64(),
ConstByteArray{"IPscVFc+EwoV1DuQLsH5G0zyaIrV7u+wB2fBqE/adH8="}.FromBase64(),
ConstByteArray{"LmEal3GHatyzQhfPXmmu2kiIoP/XSBOY7vhDM3lvi2o="}.FromBase64(),
ConstByteArray{"IGnVg4OrjumSIk09vAPoG2jyQi0BKmqJ5JFsm544Bb0="}.FromBase64(),
ConstByteArray{"qq5CTo3x2ltF3QVFha6TL0FrBNCIwYFIMKFiBzEq/lU="}.FromBase64(),
ConstByteArray{"iXNrGPgZRR/q42cq2FZk350kCOPoHN0qayNviqeR7C8="}.FromBase64(),
ConstByteArray{"E84dGafHXG8M63A0jcXH3NRwzUIV6GATjaHZ52yH7EM="}.FromBase64(),
ConstByteArray{"VTJJByn7irfUs/+ucx4hln32v77qCz6D4VlglPFnJh8="}.FromBase64()};

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << "<count> <filename>" << std::endl;
    return 1;
  }

  auto const        count       = static_cast<std::size_t>(atoi(argv[1]));
  std::string const output_path = argv[2];

  bool print_addresses = true;

  // Generate identities that are evenly distributed across lanes
  std::vector<SignerPtr> origin_addresses;
  uint32_t populated = 0;
  origin_addresses.resize(256);

  while(populated < 256)
  {
    SignerPtr certificate        = std::make_unique<ECDSASigner>();
    certificate->GenerateKeys();

    auto pub_key = certificate->public_key();

    // Lane index when 256
    uint64_t lane_index = fetch::storage::ResourceAddress(pub_key).lane(8);

    if(origin_addresses[lane_index] == nullptr)
    {
      origin_addresses[lane_index] = std::move(certificate);
      populated++;
    }
  }

  // if you want to populate genesis file and this file to match.
  if(print_addresses)
  {
    std::cout << "public: " << std::endl;

    for(auto const &i : origin_addresses)
    {
      std::cout << Address(fetch::crypto::Identity{i->public_key()}).display() << std::endl;
    }

    std::cout << "private: " << std::endl;

    for(auto const &i : origin_addresses)
    {
      std::cout << i->private_key().ToBase64() << std::endl;
    }

    std::cout << "test" << std::endl;

    for(auto const &i : origin_addresses)
    {
      std::cout << Address(fetch::crypto::Identity{i->public_key()}).display() << std::endl;
      std::cout << i->private_key().ToBase64() << std::endl;

      ECDSASigner reconstructed{(i->private_key().ToBase64()).FromBase64()};

      std::cout << Address(fetch::crypto::Identity{reconstructed.public_key()}).display() << std::endl;
      std::cout << reconstructed.private_key().ToBase64() << std::endl;
      std::cout << "" << std::endl;
    }
  }

  std::cout << Address(fetch::crypto::Identity{ECDSASigner{all_private[44]}.public_key()}).display() << std::endl;
  std::cout << Address(fetch::crypto::Identity{ECDSASigner{all_private[45]}.public_key()}).display() << std::endl;

  // check for balance easily with python script
  std::ofstream output_file("testme.key", std::ios::out | std::ios::binary);

  if (output_file.is_open())
  {
    ECDSASigner signer{all_private[202]};

    auto const private_key_data = signer.private_key();

    output_file.write(private_key_data.char_pointer(),
                      static_cast<std::streamsize>(private_key_data.size()));
  }

  // Now use these to create TXs that are entirely within 1 lane

  std::cout << "Generating TXs: " << count << std::endl;

  uint32_t threads_to_use = 16;
  std::atomic<uint32_t> total_generated{0};
  std::vector<ConstByteArray> transactions;
  std::vector<std::unique_ptr<std::thread>> threads;
  std::mutex transactions_mutex;

  // debug.
  std::vector<fetch::chain::Transaction> original_txs{};
  FETCH_UNUSED(original_txs);

  auto closure = [&total_generated, &transactions, &transactions_mutex, &origin_addresses, count, &original_txs]()
  {
    for(;;)
    {
      uint32_t to_generate = ++total_generated;
      uint32_t lane = to_generate % 256;
      //lane = 45; // and 52

      if(to_generate > count)
      {
        break;
      }

      ECDSASigner signer{all_private[lane]};

      if(lane != 999)
      {
        // build the transaction
        auto const tx = TransactionBuilder()
                            .From(Address{signer.identity()})
                            .ValidUntil(500)
                            .ChargeRate(1)
                            .ChargeLimit(5)
                            .Counter(to_generate + 1000)
                            .Transfer(Address{origin_addresses[lane]->identity()}, 1)
                            .Signer(signer.identity())
                            .Seal()
                            .Sign(signer)
                            .Build();

        // serialise the transaction
        TransactionSerializer serializer{};
        serializer << *tx;

        std::lock_guard<std::mutex> lock(transactions_mutex);
        transactions.emplace_back(serializer.data());
        original_txs.emplace_back(*tx);
      }
    }
  };

  for (std::size_t i = 0; i < threads_to_use; ++i)
  {
    threads.emplace_back(std::make_unique<std::thread>(closure));
  }

  for(auto const &i : threads)
  {
    i->join();
  }

  std::cout << original_txs[44].from().display() << std::endl;
  std::cout << original_txs[45].from().display() << std::endl;

  std::cout << "final debug:" << std::endl;


  std::cout << "size now: " << transactions.size() << std::endl;

  std::cout << "Generating bitstream..." << std::endl;
  LargeObjectSerializeHelper helper{};
  helper << transactions;
  std::cout << "Generating bitstream...complete" << std::endl;

  // verify
  std::vector<ConstByteArray> verified{};
  LargeObjectSerializeHelper  helper2{helper.data()};
  helper2 >> verified;

  std::cout << "Count: " << verified.size() << std::endl;

  std::cout << "Writing to disk ..." << std::endl;

  // write out the binary file
  std::ofstream stream(output_path.c_str(), std::ios::out | std::ios::binary);
  stream.write(helper.data().char_pointer(), static_cast<std::streamsize>(helper.data().size()));

  std::cout << "Writing to disk ... complete" << std::endl;

  return 0;
}
