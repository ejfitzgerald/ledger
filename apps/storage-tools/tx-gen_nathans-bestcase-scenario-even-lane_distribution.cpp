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
ConstByteArray{"+kU1jp6e0RbXFRvJk1s3M7Q76Ky7rorJAkev0hmGLKI="}.FromBase64(),
ConstByteArray{"1m6J4l+hTfNWtRtKf3GB49cUYo5rSTRw+f7yIQu0yKQ="}.FromBase64(),
ConstByteArray{"tU24uFOJf9lge0JZqysGgAMO4v9p7n16s5rCCp0R+Ug="}.FromBase64(),
ConstByteArray{"dPLvlGNuGk1eR2xNTdoCOFW6mi6Z3/eg7Q5m6DdbnKE="}.FromBase64(),
ConstByteArray{"MzMS7jp1vY6gBMeMNKuNCN/JyTPdQ/Ueji/fkuRco0U="}.FromBase64(),
ConstByteArray{"W2nArIalMJ4QJ4MLjZfVxd615CpSOVQnpdp4FsxEIoM="}.FromBase64(),
ConstByteArray{"R2IwQyEls5ZY+UyWYapQth1AbsqoPcRZwR1h3U1VnpY="}.FromBase64(),
ConstByteArray{"nHXkqFV5Yvb98suIfJTkQkL+gil/D5dN4bxGzud8zuA="}.FromBase64(),
ConstByteArray{"eOTs38wn4K1uaOcxongzU6sgwjpecCCcC4IrDnrOsXg="}.FromBase64(),
ConstByteArray{"/Yk9dFWT+gQxiweGSE3zkABurHvCokUt8PVHteGsv6g="}.FromBase64(),
ConstByteArray{"VbAcSL4fyMVnggWGO/Ws6aiXoPJlgiNVs4uUULuIguA="}.FromBase64(),
ConstByteArray{"q7vcEWgX6xuZxxgQuxm2VCJPiUIVRkSZxjYkDk4t9LE="}.FromBase64(),
ConstByteArray{"wiLb+wz1gkfsdxkf/YkZLwbc6Dy/mMpe4+xEbNvacsM="}.FromBase64(),
ConstByteArray{"G0RnYWmGcDvseqb8tftMufmIl7PKybp9wJI1UFi1+u8="}.FromBase64(),
ConstByteArray{"7HlFeAasZIVfIx1sd/kkLraJ0+Fa8QSJ3FlkBM69Hko="}.FromBase64(),
ConstByteArray{"3abP2briRgQKnw8dOBe2RhjpEU42SenXX0EU9Rk+bEI="}.FromBase64(),
ConstByteArray{"9jdA64FJI/Z8thVuEpDJF5RxJLUMIcBM6oRIcW4BZXQ="}.FromBase64(),
ConstByteArray{"lNTw6D8RjJlVN4bvD6byNIHN7hB9iP44/P+/r5Aut30="}.FromBase64(),
ConstByteArray{"S6z5ps2Zd2CUQMA99JLUQCY2QRTrCZ42iDGOqgnvmiE="}.FromBase64(),
ConstByteArray{"tDEpW3tAqzVP18kr7rwlQDwPGin65Tu/CNyHodHpe0w="}.FromBase64(),
ConstByteArray{"WTgkMGzK9cVtgMX8ZyzwJRVq1l6oZLnu/KPlvBDmHkQ="}.FromBase64(),
ConstByteArray{"U6ir3uYn0JdWNTb5Ydg3vAdy72ErVegju82IhK8HpuU="}.FromBase64(),
ConstByteArray{"uWHxmhLnQc/PXsjW7VSe2c+RFyIq0qCCAyBQmbZU3Ew="}.FromBase64(),
ConstByteArray{"3/z3Oy5e9Gvx9mZbZ9DShxlfOgXhPxpFjHlRR6pLCkY="}.FromBase64(),
ConstByteArray{"Cf89cRnCwQ5xbZgmFJX9eoITHnOkQZ6rClOrCxcCgiw="}.FromBase64(),
ConstByteArray{"HgBEgnqCQG+2FYmt6G+1sCmsLXmpyCnDfVODzWdzJpk="}.FromBase64(),
ConstByteArray{"KJz/fSB2S2nTh9kjbq8QBfErRKmEg/bSH8Tr7cXrddY="}.FromBase64(),
ConstByteArray{"uGAppcsJix6e/jY1L2WLalniNIP8gOl1UaDbo0Rs4c0="}.FromBase64(),
ConstByteArray{"og2l9p/F+FchT0JlAZ7rcB9UM9kf5j59O2djFNVRi5U="}.FromBase64(),
ConstByteArray{"6uVHbDvrQxa+r20NM2PBOX06NbbykS2jSqoAJWOtZI8="}.FromBase64(),
ConstByteArray{"gpxmbAP41xTFaYRULmLH53kboObTslqVfIlp/gP/05M="}.FromBase64(),
ConstByteArray{"PvvHrLEnCLKSMGUZ8Go17a+UTWqciBg7dCkU4mE2nRk="}.FromBase64(),
ConstByteArray{"1Wckvu64B9KBtALu+hLlvIvXQ6k0Qjar5nYNEK+Wd7E="}.FromBase64(),
ConstByteArray{"ygqw40ks7x9e2SORkcU1t+nEs1E5cvmPzWkPxYxd2mc="}.FromBase64(),
ConstByteArray{"wwbFX0eEJsrjnydN1w5NmOVY2CGHB2TqCMFd846WfhY="}.FromBase64(),
ConstByteArray{"/zNDhjIXtQLSRZsYJ0OhhXbglMMv318wv3BrSE8kiu4="}.FromBase64(),
ConstByteArray{"xT4JXhZzsR0hWBKEJoz6ZNfJhw1jmTiBkZJaG84/0vE="}.FromBase64(),
ConstByteArray{"FZ4CFHHPDc/fUnrbya6WrYuuT+we7OcieKx/FQuGoBg="}.FromBase64(),
ConstByteArray{"gStqVtupFYWRt0LknxViS4NCXfVbRWdb2CP4Y3HVozs="}.FromBase64(),
ConstByteArray{"K+kSuel6/7k0mRnzizCX8ijg53HFG34SjJhimwmjIJo="}.FromBase64(),
ConstByteArray{"tIvkJfbq1s92I81UrHy8FBpKPQ4s8KciuZKJf40tzlQ="}.FromBase64(),
ConstByteArray{"Nn92xP4vAOj+LL4uMx5PprX1aScBujvNEY2jh5Z+1F8="}.FromBase64(),
ConstByteArray{"cZOSjcqtpJKXFWFqmWECaWkr3ruPujTHA1TE6HO8v38="}.FromBase64(),
ConstByteArray{"cIESdyD99EGMyuWsCM2sUyxv8HP2zFKefMI2IQZSBgo="}.FromBase64(),
ConstByteArray{"Di4aBS28IcaxXK5I6ijS4k0xgKJ8lvRAMCJfAWvM+LE="}.FromBase64(),
ConstByteArray{"vyLn5KqUFLtacMkBujLB7fb3NvX0Alo41Qht/7jsxH0="}.FromBase64(),
ConstByteArray{"tPcb1pLFc5Xu8WKc33tBJJ9aqqMBiQm/o9KB7Ie5QKI="}.FromBase64(),
ConstByteArray{"HhNML52Hl55LDxqjiZvrL3bVu5Je+6+X6u8VtnPWMcs="}.FromBase64(),
ConstByteArray{"ir/Hq36SrRDQYLo7QaW1o3a4M8wHRB8SgoHCYKPQIUE="}.FromBase64(),
ConstByteArray{"5vETKd3pDXk4MoBMso82pHi4XUNXdSQmfZycUAZMK8E="}.FromBase64(),
ConstByteArray{"Tra1xgGwZ++YGyn8Dfz4DnXTcMMH+A6C8KZhirHzAzI="}.FromBase64(),
ConstByteArray{"vGf6ttls6kbeKXK6u9YWXGJUhkDD5HmVY3BbQVZIsj0="}.FromBase64(),
ConstByteArray{"4Ii5M5Mm4eX1Hl9BwoxrYWDvbUTWBVsXMdk1WcVPpMo="}.FromBase64(),
ConstByteArray{"R9t1xskoHhTXQZBc/bezVHIccGi/crqiVFmCb1LLrnA="}.FromBase64(),
ConstByteArray{"a5JjkL1wTTpt5dpMEmnV7OsMmNhOIUJjiwBLqptu26c="}.FromBase64(),
ConstByteArray{"JS2eN7VOK9vUgBssgMxCpf4bO8E6QPTBiDXHmU/WMvU="}.FromBase64(),
ConstByteArray{"ADJBSmWBXtv+g9X7MJx2qnf6jRYKWq9CcDAjXMUOSU8="}.FromBase64(),
ConstByteArray{"iETgtPM0n8OZFCWtbQLqQY2Om001623ilNSiaR0FHwc="}.FromBase64(),
ConstByteArray{"pivxjN3PHlBrPjlodTnSsFQfSH6IQNnajoMwMqEEj6c="}.FromBase64(),
ConstByteArray{"Jmc4RxzZchVu502SM9qxPoj1IFa7T6iEO3ozSb8nwXc="}.FromBase64(),
ConstByteArray{"3rqneI+uqdQ6c+GfDxknY1yo3VdGBPgye+Q7ZpZr//8="}.FromBase64(),
ConstByteArray{"fw0unFK66e0Ibi3PMmfQLUd8B0HmBuDxsmrenRO4BnM="}.FromBase64(),
ConstByteArray{"K1wyTm26tIiv09WWlTXzmBawegHraC+G+UvLvgqEgfc="}.FromBase64(),
ConstByteArray{"S9+PjFCoj+j6FJrueuvXLt3VWiPptOGSRR0iYvH2BAo="}.FromBase64(),
ConstByteArray{"7b1eaMY9g0mx1MS5P5fvSNpr3H3Ut20OA83nW0u9zso="}.FromBase64(),
ConstByteArray{"34lT1OnSZEgsgTnk0cr1ozOP5lU6Mw/yNIwTvFhvTnk="}.FromBase64(),
ConstByteArray{"J29xzltADNunqVthYQWO3mW7UVHt8y6qP55gUYqIpsk="}.FromBase64(),
ConstByteArray{"IWqyx4pwVgS5SnPumMfM71d3Fka6nfsxZJXpJ8Y5Do0="}.FromBase64(),
ConstByteArray{"p4X3LluJVf9MM0l7B2jglKFOrezaZP2G8B9ZwNuZIRo="}.FromBase64(),
ConstByteArray{"G7a3afcy/qdHeCOLmWw8083umg4o54XR47GA4XI5Ays="}.FromBase64(),
ConstByteArray{"ORFwsSX2OPYRWE3ztfCwp1IppT7zq0el58Krrb4Tinc="}.FromBase64(),
ConstByteArray{"Jj7BROaS4gNfa1Sowed6CLSCcVMZl7bagS3XLvnazWk="}.FromBase64(),
ConstByteArray{"GKGkZJAogGayEK2rFtg1czI2mm9gdbPQA1xUrkMlKsE="}.FromBase64(),
ConstByteArray{"Vkq0lmiJ5ZbVMZLyN0aFhIPbStyxVDw6u0UQ672dee4="}.FromBase64(),
ConstByteArray{"dX4xd0Qgbvj74i32AKRbRoULf+tGEhIPHWspNMAb4jE="}.FromBase64(),
ConstByteArray{"utcfLqAVdnkT/zJRqOZcVbxWSUg55a9IpV1OJcUUors="}.FromBase64(),
ConstByteArray{"meOZyP0ybZWC5j3htHqkPVbzsjN/7SC6e+W9K9uTkxs="}.FromBase64(),
ConstByteArray{"aR1D8qgqdQFQyn2H3ozZRf0u1zcHy1lx6/O6EE0CpWY="}.FromBase64(),
ConstByteArray{"WiOU9WdHE8Tb3DZ5nxRX6OULordFltNTqqqF0MYG4JU="}.FromBase64(),
ConstByteArray{"pwHcKCJaRIPbIZCv/GHmSW7s+GUTxqjNJEF9npxG2TE="}.FromBase64(),
ConstByteArray{"ZZNMYjLCHATG49WnzwXs/SIpUwlHOMkMERBTfZ5SXNA="}.FromBase64(),
ConstByteArray{"iUi3txmo9DHmjDC5BE59NZ6677JsQeLHSQRMKJynHKw="}.FromBase64(),
ConstByteArray{"rkcoalJ5IbYcZDhxsoaBHCE/dAxTSSqhYl/qlAmUmBs="}.FromBase64(),
ConstByteArray{"PP4TF68fU7X0rvfqXRmI5apSp506JHCxejp30ywy9yc="}.FromBase64(),
ConstByteArray{"8WOqV4uSCwmljarOLQd/fC/w3mnSaGqwmTlJI8Y+vGc="}.FromBase64(),
ConstByteArray{"RO/XQfFh9qL6lxm1pQ7R+nF27C4ZEvizKSF2oTqEQRg="}.FromBase64(),
ConstByteArray{"PX8x7Un97fAuDGA425Iipprbv2J7K8SQ9m5KbCMzvHE="}.FromBase64(),
ConstByteArray{"YZ+M17gtUktroOoe7CtkPjutRu3zyEViFVpPB9gMJfs="}.FromBase64(),
ConstByteArray{"7uSiQJeW5zky+hrTFlx5syEFJJtuXmYgIa2+lFN/r2Q="}.FromBase64(),
ConstByteArray{"pAULIvFe71Wk/xLWtZjb5lIGiiiH/9XJsZWaqi+TUAc="}.FromBase64(),
ConstByteArray{"XA1t386LXFEKPTfeOW3xJ7yNjBs674Rx0nNkSSJhkrg="}.FromBase64(),
ConstByteArray{"DSCJnsse9x2DCnGC0WZfqKfzly7J/YZmUJlJ5wQjlr0="}.FromBase64(),
ConstByteArray{"9j9/+7coI687Duil5xhsHssktjt+GSfV8At6vv94O5s="}.FromBase64(),
ConstByteArray{"rH4Mh3BcTfohOp9UjYY7SsTFKn3GfSJpi6+1drkwP5M="}.FromBase64(),
ConstByteArray{"ywiw4LUxwxrOY+jAkVYAH++7HolQWY4bJ2Xgbv7c+Tg="}.FromBase64(),
ConstByteArray{"SSSTjz2r93x3N2hWS1wOE4J5pR49RvVJlY0Y96nBAPI="}.FromBase64(),
ConstByteArray{"UnhcNr1dJ/7xvpP2oegVYgFk0dt+xs/geq2kEuibxdA="}.FromBase64(),
ConstByteArray{"XH6hfQxivHIIkt+7MzEwCaSn/xNY3poAq6/oSPuzDv4="}.FromBase64(),
ConstByteArray{"PrZDcxWOc5/yTy0iLGSl+Xgm3jZ2fjkNHbYd+Z7OItI="}.FromBase64(),
ConstByteArray{"995m8fEb3cUqgzk6DaEKyoZusRdmkyJcFJ1nLApZO64="}.FromBase64(),
ConstByteArray{"55V1r1R0jwoN7whLwvQKaIPU4d8gX9c/Sv4QrGnPqbo="}.FromBase64(),
ConstByteArray{"43ao38OApx4psiBO1LozMJ3L5Iisjl3RGkAd2iEfDmg="}.FromBase64(),
ConstByteArray{"fL68iPsS2SfHeu0SfIVTYJdrBydLQZYfBMHdDDHmBv4="}.FromBase64(),
ConstByteArray{"nxDpTtTsG+JWtRmZHHB+/55EvmFMPxHk2yS7KnnGDvM="}.FromBase64(),
ConstByteArray{"yje3oJ2tzVaomGzIf3/5ps90BS4Lw+VALr6XSdJgRSU="}.FromBase64(),
ConstByteArray{"I7lDTADQ55V9yqFMPkMSZBj8SBE8L0rwexMS0fESGMQ="}.FromBase64(),
ConstByteArray{"bb2m+U8ETfGREDV4rZNEu6nYc/+SUzRM3FAICba/RWA="}.FromBase64(),
ConstByteArray{"bDSSq0MeK5gguspxNNoL3ABcduGFGsVLTQwLoTo92mQ="}.FromBase64(),
ConstByteArray{"KpKWBgPVoZ26onTwH/KvFHmasTGbnxciuQyrt6oXQ1A="}.FromBase64(),
ConstByteArray{"gaF/8xqnINuzCK8FwyoLCreROoMLQ37p50qdY1pW9Cg="}.FromBase64(),
ConstByteArray{"I0rkQ492J6ba/aMiNM1Rz2aVkFdAr2t4dm3dN3YTifc="}.FromBase64(),
ConstByteArray{"Pdnf2qIDxU2S+YdA800ov1wVpjl6sECwGZ91ZvD5O+k="}.FromBase64(),
ConstByteArray{"Y4GAjbCd3AfC0HphMKUWmdlAc/0ZLQFvTeMyULj22O0="}.FromBase64(),
ConstByteArray{"yt1TNwxYiKTlf+2qtXOhrRCSmqE0bxAvc06F1MJxmZI="}.FromBase64(),
ConstByteArray{"715l5Jis6sNQOWNdSS37POlOGHNXDpKbM5y4zB6JZVU="}.FromBase64(),
ConstByteArray{"QRsrJ4vGjD6jCe56KGknRk2bP53SX5dLPNJFDwPcXyU="}.FromBase64(),
ConstByteArray{"P2lqLJCVQJoNpZAOE7AIXpHPkIIlt8F51sKpJ9nS4DI="}.FromBase64(),
ConstByteArray{"rjeZ4WlKKIiONs+38ri5/Svm9o1lU8txMx0Ffyy4dB4="}.FromBase64(),
ConstByteArray{"opoX8b8b1e5MiNgKFymieRact8/6xGb7mdItrWhSu7M="}.FromBase64(),
ConstByteArray{"i4nlHaftwzqjAlL8WiEA8zZC7w7x/NIUEqoEPw4YfYc="}.FromBase64(),
ConstByteArray{"3kGibYsW4oQS/H8kMgR8djlWGmgFPeB9S+F7fgsHM7Q="}.FromBase64(),
ConstByteArray{"hfMLb2PdCFSjXW25hE83J+kNECyAb579sWnCH9LHdLY="}.FromBase64(),
ConstByteArray{"F1Uz4kK/eF6otzrZFPZ/o38CC/eimViP3oTQahKorps="}.FromBase64(),
ConstByteArray{"ZqDPaC0gjtS4PqrXOx3s6kJVBt4KDkbhmiLxQDTUiNM="}.FromBase64(),
ConstByteArray{"BDYlqCCf49Ous0+tvSmfxtGXMD02syhXAWH7j4WAm5w="}.FromBase64(),
ConstByteArray{"0nWWn8oaata7A+6uzxpGUiajlBhlOybCwNyMk2y08/A="}.FromBase64(),
ConstByteArray{"ZIPF+dIIe3jclPEFkt0lHTDqbJnsPcx/PvlyVWvCzO8="}.FromBase64(),
ConstByteArray{"+tUhuGbWHr5+OI/r16jPkWZP7QWAGmcAH48JS4Gis8I="}.FromBase64(),
ConstByteArray{"/49ZtUA5j4hvys8Trzdu6FWXjoX/PwwDavYIMIijn+c="}.FromBase64(),
ConstByteArray{"RTOUoixL22U6cASA0GzcsyJt2+Z/9VAdoR2Dk+YzL/Q="}.FromBase64(),
ConstByteArray{"QKm5oPBcDq2EbdnFnMw2frLkSMhq/qoMDoEfPNEqV4M="}.FromBase64(),
ConstByteArray{"d11R1bG2+amZDTkxxwU1zFgBej8adBoqiWlgcBMd0vg="}.FromBase64(),
ConstByteArray{"94DOSAxPrwZQs5mRyOUf56iNIRxJ8Cbhl673X9Rvon4="}.FromBase64(),
ConstByteArray{"0Zpg1WaZsAIx/IcJ+L6ap/fhT3+nEA7op8jgdsTfvjM="}.FromBase64(),
ConstByteArray{"67M5w5jpB+JZLOEpyEID5+xUb5m0DAUGGO/kEMxRsFU="}.FromBase64(),
ConstByteArray{"c7QurbvuuMxqqSLgdb/X7gKYzr/scGx7nwzo78Ag9qA="}.FromBase64(),
ConstByteArray{"i96/sHLcv2F+KH3OJv3ML38xSksd8DwCxLal/26ol5w="}.FromBase64(),
ConstByteArray{"iU16/B6sOTGLu41NEWLznFAzs+FUdhUE1E/7qcw5Txo="}.FromBase64(),
ConstByteArray{"cMV6UzbivdPVVRjl7V5qecaxE5VJ9IZJ2se2zfYN4Fs="}.FromBase64(),
ConstByteArray{"bKoWwPT1REQo5VH89gMI8+yRLY5mYZSdoXo0d2uy/Rk="}.FromBase64(),
ConstByteArray{"wRi+AdmHQ4eyRPJ5VkzF8XVIm53tKNMlXkmk8sDfGpk="}.FromBase64(),
ConstByteArray{"s2BY4F/chVuVu5T228+hCR8WW5frINDRG0JhtYXwvEU="}.FromBase64(),
ConstByteArray{"dgHdcMEslhtsfaOD0fAhR1Hw9W/AI1vB5PcrsAet5R4="}.FromBase64(),
ConstByteArray{"qA/iik9WxgokpMDXy4aTkZJuz7aSDogayWakdz+VJaA="}.FromBase64(),
ConstByteArray{"3kmObtit/pay58/Bd8WTQULEoT8FllPnE1zlIJ/9lm4="}.FromBase64(),
ConstByteArray{"3qHxc4co847hxZ4hXhWG0HM3e7x6D2nAiE2xjbWB/YY="}.FromBase64(),
ConstByteArray{"mV4S3DI0/wp5bdFuvCXLeFa5GvSCoCRfekijUMh3aio="}.FromBase64(),
ConstByteArray{"2D9bzRgCvtVjzvF6PnLotBQjXVeNmiBUaXOfiMlEMoY="}.FromBase64(),
ConstByteArray{"klld5HvRa24rFKW/XzOQHGhEDAwfYgBmDJ0eEBLgrT8="}.FromBase64(),
ConstByteArray{"sC7bAhKpGtRE3eSjRxMruaSXQTs5QSUFfwEACLY+G7E="}.FromBase64(),
ConstByteArray{"DFIcSvRRvHWOK41IrIZX5M2DwrssKW67SpBYy25ZPcc="}.FromBase64(),
ConstByteArray{"MdKIOXTW1gCIF0+ENumt4xyqSFPIxTxnPt0zSEtDMl8="}.FromBase64(),
ConstByteArray{"q5rCNQ95k9KVQ3t0EYHrSJ2sf61i2s/VH4/Wr03SFYM="}.FromBase64(),
ConstByteArray{"1M+Xz2xuJj8ER1GiJBnddastqfTDWZvPFAo18ARHwr8="}.FromBase64(),
ConstByteArray{"9Gxixf+e9K22JsbegJDmkORmqsF3Oz2YHyf0m+hbaKc="}.FromBase64(),
ConstByteArray{"PQqy74jKFaZaAAtf6VV5QKrp72YgwuDZGhTlyjOKqcM="}.FromBase64(),
ConstByteArray{"E2LLKeTXUKjaWYvUQc1KzTKUaLVixf/UeShrqRqkgb4="}.FromBase64(),
ConstByteArray{"8hWOl+GJ/A5L9wVi2eHNOz19kLiChfOnZgPTgCNoClk="}.FromBase64(),
ConstByteArray{"ihJUhFwt7Ynyom5MJQXbRPq48nd7rxtJBoB1kFzl55A="}.FromBase64(),
ConstByteArray{"VY4Wq0MTY/l2UC+EPX81wp6BLO0CCUXMhjkpZjWhAnA="}.FromBase64(),
ConstByteArray{"hf+eble5oqHuKGGnEGp5lUfCDcEHJXUpUavE5zdyezo="}.FromBase64(),
ConstByteArray{"BE4ywJmLiIGU1sKEIpOptRPhNKW/YyVHpwTGORZ3fNg="}.FromBase64(),
ConstByteArray{"QSC/OZvdJIqNm9mwZN5MU7FFW+95rJcleR4NXhfQjSY="}.FromBase64(),
ConstByteArray{"QZIxHBA/FqDeoXjussaH86PZeAryARIOrOLW6nZ2sRQ="}.FromBase64(),
ConstByteArray{"IOFp+B0FYn0dHI0G045WUnRW9eIsvaBlzHCtX0dwt98="}.FromBase64(),
ConstByteArray{"xS1fVKYpJyYz0UeaGJPuckhHF8zz0uZiadjxrNkSnwA="}.FromBase64(),
ConstByteArray{"r0HbvqugEKdAAGm2EgdKBMJSuEPQtoE4S7kmz8KpGRU="}.FromBase64(),
ConstByteArray{"SrO5V+E9g3ahmCAq8+/fata2wdMrbRnaJXUXkGne4PQ="}.FromBase64(),
ConstByteArray{"4+fFfrfYuHnr/wkMh3wSzwnZxzWOItSqkIWp2NdfQHc="}.FromBase64(),
ConstByteArray{"yHtqW9O7Cuzed8dP0cWCWnqd6sEqkP2C2Bd7zHSt4Vw="}.FromBase64(),
ConstByteArray{"Rvf03BiMcRplndYgxqDrPnyKtys1iDHtqWSYkZcUQEU="}.FromBase64(),
ConstByteArray{"ycWaTA73nHcIx0VjHTsfF4vIKCJFNMQnscANbFfneOQ="}.FromBase64(),
ConstByteArray{"BA2Os/Qqjf73F0Uxv8MPOxrLYSZoSxVYoQ1zRhXubx4="}.FromBase64(),
ConstByteArray{"PsPFfqOQ6xr8LbhuF3XFwoadnOt4NXKWkl4FMF8/zr8="}.FromBase64(),
ConstByteArray{"h2ZMN8c+P0C4ijY/7fIXOBDXYphc2j9kDNg31bfs8Pg="}.FromBase64(),
ConstByteArray{"l6mDUclku9+FSRyQBma5OobN5DBdicc/OtWDAJ7ZRGc="}.FromBase64(),
ConstByteArray{"PKc7K5YP3SZD3paSb17kx27Oxb2IJectrFwAZNgDlG8="}.FromBase64(),
ConstByteArray{"IJLQPIR1pK9WPSGZO0qzHqZdcSP2tErHITeVLds+GjA="}.FromBase64(),
ConstByteArray{"c0eECSygiN7qGBxULsSTI3XPPb9xdhohSBKMVd182/0="}.FromBase64(),
ConstByteArray{"T6ONxqnGEmUhkfG779VcPowMfTeFS34qX184NVJaM3s="}.FromBase64(),
ConstByteArray{"JDMNi2tebuAh42JB3YLInYnfdNsxjGpAvF2LHYjUu3s="}.FromBase64(),
ConstByteArray{"19xRWnfbgKur/fp0JeNwQglcvw7IlMs2CZ5oHSgzP/k="}.FromBase64(),
ConstByteArray{"BGHsY1jxf/sa7F2qYDCcAfbL7MTDsxyU8aRoVo8VOh0="}.FromBase64(),
ConstByteArray{"byaOQMXQGjAXw4gzY1BbInRyqKyM+IHkPWlLzCVNaV8="}.FromBase64(),
ConstByteArray{"O9S3p3uwedLND+lFLe33Tjj5Cov5Uj08PDy74zIk9Kw="}.FromBase64(),
ConstByteArray{"+nAu99OWABljFrYjGmzVA8q8324+YZRqV9RcShmvClk="}.FromBase64(),
ConstByteArray{"JenCYrE558zz61FTtpHVMvPANCIu2PlZIEy+dxKTRFU="}.FromBase64(),
ConstByteArray{"U4/P0An40xLJuTrmEYcFnamcmMKFKXYvHzf+1zf/EgA="}.FromBase64(),
ConstByteArray{"g6Joq1URGmICyO/XPr8iMG2NyiIq+y0zAO7d9k7bM34="}.FromBase64(),
ConstByteArray{"jRVs8IP2YDyufZ4njrlsPRwxrJOGhYXAJ9vEq9qw/YE="}.FromBase64(),
ConstByteArray{"sCBdgBx/UCpWzx6FX1SkpkcShI4cm9eaVIhZVkAPvhw="}.FromBase64(),
ConstByteArray{"kfMrHIo/JNN/0oE4f/BhO1ikF9xP7yOVqgynMzDGGgA="}.FromBase64(),
ConstByteArray{"7CaukZrgOAH3fChVPOQE3y1TFE57PyGuTSqV77jtl9Y="}.FromBase64(),
ConstByteArray{"w247/Jmf+azHuxEeR7oNz84Zsj0oiAjD1zHMavNvCVA="}.FromBase64(),
ConstByteArray{"scAdh7cvdH6/ipjMygJzsg3/KisJwmjmQuMW4y9AsHw="}.FromBase64(),
ConstByteArray{"zeatcxZslvpYWwwXVt0nI4jjn3o4gXKT66O/REn9Oyk="}.FromBase64(),
ConstByteArray{"kKzLT/Z84k4w/jurtxBCqprNLx/L/PS6N+hhNJOwBFs="}.FromBase64(),
ConstByteArray{"0VHOypg6l0YtpyJ5JywutRc/+kHpelHlUsZEsi9PCm0="}.FromBase64(),
ConstByteArray{"TrOfpDiaapkWe2GSlqc/LoiACmM5znzvg5I8T8kmJ8E="}.FromBase64(),
ConstByteArray{"khcwx1K4QPSrTerzmz4eKyHQxlohNBppY1ijhAt8fs8="}.FromBase64(),
ConstByteArray{"kRBKqIylpru1GK5G/nhsDLmwODut9xKtO3m1p2dK0rM="}.FromBase64(),
ConstByteArray{"oR7Qfy4tHZAXwJaiN63iuhAig8fwDVFGrubLYnbaEc8="}.FromBase64(),
ConstByteArray{"0xDWamSgUf566hu1/emGRvWRKv+924DyeXBTNjx3B7U="}.FromBase64(),
ConstByteArray{"9+811ilJclQCPhNXncRs8MdhXh9+NmkIrfGxlWosxWk="}.FromBase64(),
ConstByteArray{"1ljCNIgXskwh3wrfc4SZBoHb3S+IP5tZbKxeM9VVHS8="}.FromBase64(),
ConstByteArray{"g9o3vCJHOYc8hBZA1ZaNr4weP3xZrosC0OUjkfIs+5U="}.FromBase64(),
ConstByteArray{"WQn9vEwbTqbNAH15VsIuaCJVeA9qgllGWiMYXNdj0DY="}.FromBase64(),
ConstByteArray{"g8mvd9wySBTG9fO5eHWMOD6ocsgWYnz7bbuNz36vWWM="}.FromBase64(),
ConstByteArray{"kLxeAeYCNHNmi9awH4lVwjIpTI9AximovBdOStXpZhg="}.FromBase64(),
ConstByteArray{"cQKH4uJx1u3JLKsx/I2lspF945m6MfTUO102cmgJHIk="}.FromBase64(),
ConstByteArray{"xDTInchMPXdHy6VgKce5FKjDfJ80cPJOPaK1d5wVg/Y="}.FromBase64(),
ConstByteArray{"FopZn/XGuMhgMsE+GV3fb1Hb4vgmdI+3znDop+LgWVk="}.FromBase64(),
ConstByteArray{"RAPH1yKcim9b4BoRMqOhyPvFekRo2kXM7P8Sf7r4h8k="}.FromBase64(),
ConstByteArray{"XG60H/2lrx3zbA16QxTLZSQdQM6WtzJIXz0j47U7qTU="}.FromBase64(),
ConstByteArray{"OhcrqNXnKMakNmvw4+lSSmk5M5qOt7Mbff2FPejBtJ0="}.FromBase64(),
ConstByteArray{"gYxQgo0sETKMudCV+DD9+43SWnTKKg8aiF0Yr03m1S4="}.FromBase64(),
ConstByteArray{"HoyupTOxhG7dKA9BazKWPZWgmvxRLAKz3bg9QveeAhk="}.FromBase64(),
ConstByteArray{"K66fUe9SQYZdtyMAPwr6AqHAUZxrmLQqoLHtPPDeFFk="}.FromBase64(),
ConstByteArray{"KyLRSMq0Yw6tjo7TwuYLYTZEYzu59nCpQHYzi0bXdcE="}.FromBase64(),
ConstByteArray{"ezds++OfYzQFkTK/vTMzNInU/FxuTzwMwFHf//TDT68="}.FromBase64(),
ConstByteArray{"ikJkLR4UA63jon2SKUXT8UMKnYNlID1TJ+tPY+uKTBo="}.FromBase64(),
ConstByteArray{"VcMQ0k2HQ+JJ4tojFSCBfB26l6MDF39eHbS/yqu0jlo="}.FromBase64(),
ConstByteArray{"ETCPVvxKfTqvN7vRjg6FCWKy9Ef5P33OuK9+nPNPhRQ="}.FromBase64(),
ConstByteArray{"QH4GwTDYdtMrXY1gpjCK/ux0eUi0G+CF/M8TPv/cxMs="}.FromBase64(),
ConstByteArray{"AmhQxva/FUb+YkXzxRj63qoTXyQiOdMib8BfU2rsoRw="}.FromBase64(),
ConstByteArray{"5W8t1dBpYZilvOoDDGsXBqw6+71QI2TkRLRsoo82myQ="}.FromBase64(),
ConstByteArray{"J6Aqp1xveJExBVmwTvO9TmrmifRnypYZ84qg1/+NEbk="}.FromBase64(),
ConstByteArray{"Z5yq2wu35ljub23qvdEeTGqrbyULj110rJBD9pqD/mQ="}.FromBase64(),
ConstByteArray{"61dOBluPTjGG0qhKPRWnaBIxn9nb2vun6frn+4W1/yM="}.FromBase64(),
ConstByteArray{"syVCg6UqGqC/Jippk8n0vfzDsP9lOxGGFakItBnGE0s="}.FromBase64(),
ConstByteArray{"weBfPyA7p2YvkNljCeCkAZs1NxRr2pijA8hhmwEdcDU="}.FromBase64(),
ConstByteArray{"CiQT0bE7do6gVWpVe6EKFQlc39LWg+uUTbc2XThQbbk="}.FromBase64(),
ConstByteArray{"6d97iVR1/Sr1dXa3ne7E6sla0ez5i0KO2cljIjrJg9M="}.FromBase64(),
ConstByteArray{"+AAdS5i8T7rwMkburwVHFv6j8Rh2/oHemriLEqnk8ho="}.FromBase64(),
ConstByteArray{"IAh2w+qB3bNkR0q3IaO6HJa9yaaehI/uqZuERkOXyQk="}.FromBase64(),
ConstByteArray{"BOhtzJfnBQjrNAls9jRg6IIBwtN3R/asea8HG/Auc5Y="}.FromBase64(),
ConstByteArray{"Rc2JHuLgBPIz8Jgnz/tYQD7NtgbzD/YitS+ysYnBQUA="}.FromBase64(),
ConstByteArray{"of8uze3vx4AxxUJ5/AuBjxzyPpaf4KjR2wPFgWNAIsk="}.FromBase64(),
ConstByteArray{"behqCAHhgpqNxu2SdF7batnm7ngITzuXdAdKc76XObw="}.FromBase64(),
ConstByteArray{"XqRHbrFeeOU+lHD7LTu4X2NaHc8ZecaXtJqfaD4IW+0="}.FromBase64(),
ConstByteArray{"iXVlneOw9G0cgmILSLpZDvIBna505yg0k8G3/1rmgqw="}.FromBase64(),
ConstByteArray{"imHXHMw7r6G2xBI9tsglHBi1pcSQc8VCm97b9dAM6lg="}.FromBase64(),
ConstByteArray{"odzMgkSFQH2E60nfC+AZYKdrDgvA1wU4u7c0Ex7cyq0="}.FromBase64(),
ConstByteArray{"dTlKBwgh5Ck7aXsndWjiMEUGknkqMH3IQHDy4d3aOvk="}.FromBase64(),
ConstByteArray{"xZNtaMQ+To2BuEz0jyg1YHfJZhPXNa00LIjYntWzmFE="}.FromBase64(),
ConstByteArray{"xrHlT1GbHKfRKRT6rPkMaMUfkkL74IzCCoQogyURhqc="}.FromBase64(),
ConstByteArray{"IXooQOZSvHJsM2KJBndYwIvl2lzD9GwjkpJ+mLNtLVw="}.FromBase64(),
ConstByteArray{"RZcb9Oa5ZGkxpuloh7oGaogPfkankyp98z3MtsipQR8="}.FromBase64(),
ConstByteArray{"ZPmoYtafgOkPcf5IVDWQHHS674Dv7OLcKwfWzM/UErY="}.FromBase64(),
ConstByteArray{"sEWGHJDPMsKP7KaMW6DP1mxzJdyXEp5YLsjmy/6TLqU="}.FromBase64(),
ConstByteArray{"yT5EYOio0HoWemH4FCpM0BWKuMcS4oiwBGUJdt605WA="}.FromBase64(),
ConstByteArray{"Vri00fjjcjgnUrS8hBKd56mw3IBOcjkze8HBjVLOlXo="}.FromBase64(),
ConstByteArray{"M2UGc9nWxRU4C+F3sU3SKzRZrjpelHn41nvTPrpdmoY="}.FromBase64(),
ConstByteArray{"wS8rITzjpeOxhTUDK/Hh03aZkFAG+9LL+k6o81GfYsc="}.FromBase64(),
ConstByteArray{"/w/u7fu8ak5IkbBEkF4bWnLyrRGexM/Aj5SHkmWfqNo="}.FromBase64(),
ConstByteArray{"+mTG66u763EcI0HOB3aYYuv1530D8se5vG4q27uw7v8="}.FromBase64()};

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << "<count> <filename>" << std::endl;
    return 1;
  }

  auto const        count       = static_cast<std::size_t>(atoi(argv[1]));
  std::string const output_path = argv[2];

  bool print_addresses = false;

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
  }

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
                            .Counter(to_generate + 10001)
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
