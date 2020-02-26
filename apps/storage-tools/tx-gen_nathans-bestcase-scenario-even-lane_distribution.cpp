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
ConstByteArray{"fBFocT4RCLOIel0AMQIuRyICy9ICI0JTIFNj/POeJ8M="}.FromBase64(),
ConstByteArray{"S6nVlHbX7DU5QprdkwiyQAj/w1pSv/tImPefIQviUNs="}.FromBase64(),
ConstByteArray{"bhv6iHhTI5Rb40dYuaxY4Abc2XkT6JriR+jEHI2YNIw="}.FromBase64(),
ConstByteArray{"KuQIEQCpuduGMnVR8fDbFWcGGKb3cCy3KQegy/opo5k="}.FromBase64(),
ConstByteArray{"gpgjDsnkaF/d59RT3bL39v2/H8pyxIy++B2yjSanZ8o="}.FromBase64(),
ConstByteArray{"DR3WHJ+ZMSSP7L/UA55YRgZLTBHyLj+8/D/kX0z8eVk="}.FromBase64(),
ConstByteArray{"iJXbJMpfTRufoEYnSMVBeozdhuQGaBolZQHb7rOJ1zc="}.FromBase64(),
ConstByteArray{"9hYuMnAK3EiG6x1HHcJUoEjBN7VKdow30J0W0Y+OIvE="}.FromBase64(),
ConstByteArray{"BizgS3O/B2SNZu2hn4AWeAn0ysJhlktAivaVeqTMHkA="}.FromBase64(),
ConstByteArray{"QkBpOfPVt7XIyKkSZaWStMJqarjXVODgmuaWBSePDYA="}.FromBase64(),
ConstByteArray{"Afi8b/W3hMUtjf71e71o3juSFcUZ/nexEIjHgxOLzlA="}.FromBase64(),
ConstByteArray{"Wweji338O80fVi8HQRgDmYAkipEI2VmuQWcmX8D+Kts="}.FromBase64(),
ConstByteArray{"WX8L18B2TXm8H2exHnjkqh95yThpXzzI55lwKJS8obQ="}.FromBase64(),
ConstByteArray{"LTTzhDnf/1/X54CFqNHQ18KR1G0H7RYT/wDr+X3LLOs="}.FromBase64(),
ConstByteArray{"GMkPbaE260Dt7kKVZh9uikR8bLUNL9J4cdlBt8w3dRc="}.FromBase64(),
ConstByteArray{"YqGs8i692sQG2MtT//da+ITSOZ02GHXrhWUP3z7QTFU="}.FromBase64(),
ConstByteArray{"6PCEnhLxskLZc9WOKz/dqFJrSxOOMxvuV5DzU7nLA5s="}.FromBase64(),
ConstByteArray{"tRgMM0+5TrGQBgGeG026jqynKYa2O21wZH6rcW4Le78="}.FromBase64(),
ConstByteArray{"OmCgeJuK+ramyJk+xdiITB+h6fP1xUNlE7TOFG3qzVI="}.FromBase64(),
ConstByteArray{"kfDD0j9qI5HH4//oFuX9+ncQ8vl6q5XF6PuMuHF+tgQ="}.FromBase64(),
ConstByteArray{"FqZgTL4nVAfQF6F0omAomQ1/iAgNEkogeQDyAanpU1M="}.FromBase64(),
ConstByteArray{"s9JlsLt9IDz1uqwZikd2xFlFfe9Ejz8IUEtMEy0rwxw="}.FromBase64(),
ConstByteArray{"svhzJLHUB6EZrOURDAMDlGn7IWaT6Cy0RetehcdqpKs="}.FromBase64(),
ConstByteArray{"iteRWbLoxToAnxbGGNz7GCHCSDW3sDzXU7A2a3C98YQ="}.FromBase64(),
ConstByteArray{"RmfIobOoQmJzjG+dPJeDKWkxwuZB9J+DxKVrxHhJtcY="}.FromBase64(),
ConstByteArray{"pc1Ulv9BGubl5aGjZKjq1zTOqnayZcUw3tIhoaQk/sA="}.FromBase64(),
ConstByteArray{"NBbmgWYylkHdFIU9VZirL9AO2P7JC7qEMSHe8/1drzk="}.FromBase64(),
ConstByteArray{"eyy68kECQAyZw4q+QEjHZUOp3JXxxSDY1sXhci3M7cI="}.FromBase64(),
ConstByteArray{"4ys9YQb2FtXqzdAJ0+9zE96EI3OjZvEMQeg/xt6rSwc="}.FromBase64(),
ConstByteArray{"QP37hCCFnFQK4iuUy8B8NH4yz2Q3L4EW5zW80ZfXwXQ="}.FromBase64(),
ConstByteArray{"WxQKte5qT84g9kaXwNW1DVVhwCL0e1F4qUEoRVEubqI="}.FromBase64(),
ConstByteArray{"RZC2ahCTW46X3KjVIloSiDmVUaGYHIFwMiQlmPJ+TJI="}.FromBase64(),
ConstByteArray{"ZvJL2jsA4IlLuDko12oFEO+/9Nze3odhptPztTZCncc="}.FromBase64(),
ConstByteArray{"LXUHkfHeKi/eSLqn2QY5I0mEFcaR8Xm+7SD5LWAYs/s="}.FromBase64(),
ConstByteArray{"L2I78LS4dwRBjNbsRbpkUOfLr5AjFZcQbzH3SPeGCLs="}.FromBase64(),
ConstByteArray{"RG/XolbY2dXe0iliUN5CY99QmTmYH1T2FwfVLRADUiY="}.FromBase64(),
ConstByteArray{"oEvs+gWaccxsqU7MLP/JKbA2fXo3N1PEePkUv8hhwwQ="}.FromBase64(),
ConstByteArray{"Sm9CDayF+xKyhEdOEi9vrobdLoUf8yMrQBeEzVz8nNY="}.FromBase64(),
ConstByteArray{"/GuZ61sSthpdZhcnKDWaBkobZS+qa98eQ+iStT66MvI="}.FromBase64(),
ConstByteArray{"TIUGrDeN/N+44pv6owSLESHk0Bb+Q93aJZHOJd2TY8w="}.FromBase64(),
ConstByteArray{"RHii+z/teH8h1RDbeQ2TC5bENDRZ/yIhS0PuBhY9NSg="}.FromBase64(),
ConstByteArray{"5BonYExIul3C0cc8Y7kR7OH3fpc4thcMKnla+eao6X8="}.FromBase64(),
ConstByteArray{"scQo32GVkxCoVCSOx5P7nBCtSZuWzFAHV/efkd4RPso="}.FromBase64(),
ConstByteArray{"XczMSrbf5EpbEFayO76VWf6pPjeXMDkCDLcuHzYwDV4="}.FromBase64(),
ConstByteArray{"wCZ/W53Ff/aE8hZZA3ha5p7+vsZUR2UB0HmSB0orQ8A="}.FromBase64(),
ConstByteArray{"tHl6MfSWZ7x/fJmQAVJJoEybpIF/SUm9XzBk4ggPjss="}.FromBase64(),
ConstByteArray{"mdBIG2uWYCQQ3jjvcSNba3/ooO+efxFmrK2DiKjY1nc="}.FromBase64(),
ConstByteArray{"gNrgVW+UZliODWSTa1jUrY830CX/7w6ESejbEq8O/DQ="}.FromBase64(),
ConstByteArray{"BKww60wCTo1BrPM15Q74nQiFeosJbtHkNZ19WS1addw="}.FromBase64(),
ConstByteArray{"VaZuhq+FWiDYLmmgnJp5bZsRWzyvR1zZ7jmjre5Uq3Y="}.FromBase64(),
ConstByteArray{"mvF3gtJ1BaQlMThwB9Y5oe7nC2KJjInUjOH6MFYPvWE="}.FromBase64(),
ConstByteArray{"qW5BlPhVqzZ4JnBoK6EUrxdeCOjBtx1MqJXUK4mxyWo="}.FromBase64(),
ConstByteArray{"KKkRjEAFop4HgveSKdqML1k+vgBDUZ1tyY1agN8OIBk="}.FromBase64(),
ConstByteArray{"tcZhWmtzoRq9tLSgltCe295c5d/wDlVWfdldFnZauNM="}.FromBase64(),
ConstByteArray{"4y1grLX+49lgs18Rw0z4C5CgDceig6T8MHD2uHTRnBs="}.FromBase64(),
ConstByteArray{"+0+4h+Xaoh/MzWYwiPrc86seyvr6kHYk0SDc87i/V7c="}.FromBase64(),
ConstByteArray{"WIcy2pfsXRF/q/YASu7f3++F+bSXeFVjS5DaDdX3BjA="}.FromBase64(),
ConstByteArray{"KF7g65FSLKC50+FKztdrFBRRVCF3ThF/ky+sfb24+GQ="}.FromBase64(),
ConstByteArray{"KhtLcDyBrNMdh/YjN4zoILxEu6OQ/T+Mq0mzfMy/Ec8="}.FromBase64(),
ConstByteArray{"WNsoy7VI+8+oJNbqKozH7eeM0pJDFYntus615wU+YWM="}.FromBase64(),
ConstByteArray{"bvaXPY3CMIU4QezpbnV8al0qTd2tcJpz7oqb4FAjMEs="}.FromBase64(),
ConstByteArray{"cRaaKMjES8HwUjLvIMitAUWCLTTQPuT1KtYEyLSC/Os="}.FromBase64(),
ConstByteArray{"q1Cy019GSoiOsMhxxOMxPPKKDyHHSobacxB5CmCPKLw="}.FromBase64(),
ConstByteArray{"zmHX6094T7L17d0MZ2Xaa3PUd/PtuPYyT6AZZfJKdpk="}.FromBase64(),
ConstByteArray{"R5ntA6uUqQslX+EVJWnpDNh4K6etsQsCJzGhAJyujB0="}.FromBase64(),
ConstByteArray{"njOkF6S2gMvr+B94qrXDoku6Hh5VupUWFv36z71kKZw="}.FromBase64(),
ConstByteArray{"0gW0fgxagjVoYlJUiUU38nL3+37vJ1BjDLZaqs9azTU="}.FromBase64(),
ConstByteArray{"pb3JprBOME5jYeFUY1lyhFaQBaRilSHHYrNOPMnjL60="}.FromBase64(),
ConstByteArray{"LTf3oiDD24RuiadZmXvSoUkqTSuWpb2LNOpzg3NbuB0="}.FromBase64(),
ConstByteArray{"r+u9vfN/G8235Rm+7VSg6tKi1wRj1T95I7R6T0Zx8Zc="}.FromBase64(),
ConstByteArray{"JTc6Nrw0Nh9mzWjj7a74dKCQDtLY24cSozxc/QwoGQg="}.FromBase64(),
ConstByteArray{"av2qv5MaD4uOwc/BM1/UZgR+SUm+TWWRidUiIHt88j4="}.FromBase64(),
ConstByteArray{"urmfr2Ahb/H3m7gIKmB+cZhtdagttj9/XNj+eNUJhkA="}.FromBase64(),
ConstByteArray{"TUYEioZLfbOnt7k6g20hGz0Ld9UUSLlvkF0JN8u4org="}.FromBase64(),
ConstByteArray{"6a3T+QCCW0vSfyy6RHB3k1+ITuxMsWqfUhexC0JSol4="}.FromBase64(),
ConstByteArray{"IW3Xehb9XzG5icwY429kPFvu+AVyDuHnNLtPuPTMfHY="}.FromBase64(),
ConstByteArray{"UslChsgDEn8vcCSPRoN7W3xQR/udROegt4sRC4oz0z0="}.FromBase64(),
ConstByteArray{"huhsVksdLTVv1AO+5lD/e2msZpTNeFo1b7njMDxg6pc="}.FromBase64(),
ConstByteArray{"HKCtDR9AewTFt2y0eHfAwvP6UIYgDCPNKZvbWUh9U0k="}.FromBase64(),
ConstByteArray{"SF950f0UvYuXgnf5QmnJ7GMrOodAL06z6Rv49VCWEQ=="}.FromBase64(),
ConstByteArray{"n/YKSl1h1/yLUhygv+HxIvQACm/T/V2f+PxAVOa9Ams="}.FromBase64(),
ConstByteArray{"cdKnb4bc0Sb+/STQYf3Qz8y2woPzbaDQug3hjQAUHZo="}.FromBase64(),
ConstByteArray{"F0zD/w9b1U1xPCNCIny1s0KCR2b4EYsqgVvhvHDgjQo="}.FromBase64(),
ConstByteArray{"pXMzzpGsmf8ziKY2uHdUKFIB0XELfK3JO7d5DT/08TY="}.FromBase64(),
ConstByteArray{"RdcuRNiO6jSBNbUp6g5m3vP7vhpOMvYf4BwWoLj73TM="}.FromBase64(),
ConstByteArray{"7aVT24+eaL5xcayNyFCCtXaV0Gs5qf3qjErHE+dwNnk="}.FromBase64(),
ConstByteArray{"cGblOgbwRAj3iFdKyVrLwb8KPYIEhpeS0SgPVqmGf2E="}.FromBase64(),
ConstByteArray{"YDKTkIebA7YvbDrckDcu+J9jGV4NclcUqnCiJ3Bjr3U="}.FromBase64(),
ConstByteArray{"MzSVtkLySO+riwJwPshqUi+2tJz6BHvXFpBu9Yk2dBQ="}.FromBase64(),
ConstByteArray{"UESf3HZdAT4SxGl0V8aueIlhK13JdsBrlMX72EjHtlw="}.FromBase64(),
ConstByteArray{"tKHOzAGRdzEMp/wgYhsdIF9aC2fUw1P8rolIH79iTMc="}.FromBase64(),
ConstByteArray{"Fpumryk2d4R4B8J7CFkc6fgJ9YZf3R4v+fAqiw+hlNg="}.FromBase64(),
ConstByteArray{"SMGCrjNSvrNUDFQHWkAJ6hFWTfxn6zdtfRhWyAxNXko="}.FromBase64(),
ConstByteArray{"ArKblCW3dFgceX1gGiecXC1ewdTF+SgZF8kq+Wq7EaQ="}.FromBase64(),
ConstByteArray{"7K4U7fJRKXeWS7AyRqce9cLewk0cRYekfJXG4R4Orlk="}.FromBase64(),
ConstByteArray{"iQ7RNIxFonoqFWqkKR7b7mhWDk2XFCaxr5X9DQcMOZo="}.FromBase64(),
ConstByteArray{"zn6T45vgwt9IDC+QmHpzTIkyoYAS6k3jGhN0vKxclxw="}.FromBase64(),
ConstByteArray{"9k8dEGwJf0GYaIfaq/89Qd7JqsG3SnPWQHvxuoN+xBo="}.FromBase64(),
ConstByteArray{"k7r0pvHEIDBtnb2qzuo8plBAe8xrEK2WaQXLc3P9Fqc="}.FromBase64(),
ConstByteArray{"mAuz0C9sQ/wfZZkz245zM6Zor+fuFYQrzR159aDbKtU="}.FromBase64(),
ConstByteArray{"i3awu5jWlxG96x8yX68aEh5KLgyzgcKe/HPc8I/i8mM="}.FromBase64(),
ConstByteArray{"ipioBZfjruWuIRCLEyHB5MXQzeDJ9S5H/1hPgF4Yhls="}.FromBase64(),
ConstByteArray{"aezxxf90mST5HaSv83Xmqz2c765zO83wAJc0HBSk1nQ="}.FromBase64(),
ConstByteArray{"KWxmYa7ArL/YmVm5HGb0lkIFkhcusyr3SAHLJn6gmOA="}.FromBase64(),
ConstByteArray{"ZQjK5XY/trVcYiha9me6v8t6N+1jdZO0bB8K0suokmE="}.FromBase64(),
ConstByteArray{"4VGql+8OsNYoWUIL0qG/KdCtqsng1/TR8kAoMUVX1ds="}.FromBase64(),
ConstByteArray{"w2H3cLdk7qjMXs5l1gxgzPP3Gf36ZgoMyR2ZzolbDYQ="}.FromBase64(),
ConstByteArray{"gzQrXK1e1kTvF/YiNnH9+DFUlmEkA040XCHRbfhDyrc="}.FromBase64(),
ConstByteArray{"wU70nQZpUBjf2OmT5G45q1hJ/91S5FWYiJP3t0rEdc0="}.FromBase64(),
ConstByteArray{"45CT1HcPs0sEkt/rWr+Q8EBoVGS3B0Emo24v6sH03yY="}.FromBase64(),
ConstByteArray{"1Wl8Gps4SEtztjuQ5v1Ro88FRtAQot9gzgN/X6xBwI0="}.FromBase64(),
ConstByteArray{"3ZINO5zO1ITTvbxjRzYh23n/OhPCAETY1E8fPyX9UZ0="}.FromBase64(),
ConstByteArray{"9k9Rlkbe1PSO+egBunFwYazPqLjKhf9J4DvBt4Q5eZ4="}.FromBase64(),
ConstByteArray{"JL4rDq9n0mH0R2kscIxkJ0nzab90a1aVbVTlDFHIgtQ="}.FromBase64(),
ConstByteArray{"L9RY5vtnGVAnykyBSMJjwPf6uAIIvpE22rJPAKVyGhs="}.FromBase64(),
ConstByteArray{"V/VDhdSY+V8u9leOArQYFSwnBVczxvG1ER2aFIoJTgY="}.FromBase64(),
ConstByteArray{"BXDU95pTNqw5Nergs8UtoxTOyM96f3e5k8uRJSHUlJI="}.FromBase64(),
ConstByteArray{"L6k1z2XpMHB3QmsPArW2w44Hbm0r6rn/GO6hha6JyFA="}.FromBase64(),
ConstByteArray{"HpIMnmVMReNmyU7I1nuUxTs4kYAxhtPAOciSvP2S5tU="}.FromBase64(),
ConstByteArray{"MSJHhqrSamOybjgaK20Mwq2mW3Z47BoR+DUMcnddLKQ="}.FromBase64(),
ConstByteArray{"fRaQNXnT/b94wpqJiI8HbCN4T+r0VZHPzxv0LKUsQ4c="}.FromBase64(),
ConstByteArray{"1ON1nsMLjTtmmCG964uKA9F6RBDCGbmzmcwCkOMwPP8="}.FromBase64(),
ConstByteArray{"NPOdstnFdGoUmvYgWySUIf24VCk894FCrI3zbZ2/Yjg="}.FromBase64(),
ConstByteArray{"WRxoby9PSudnOT5d5zQLKsoktdN6AmIJh9JcB2XZOl8="}.FromBase64(),
ConstByteArray{"8yrhfhTb5sVMuiDXNVPKcJiMlsxmuKPVsSJuUHpZh0A="}.FromBase64(),
ConstByteArray{"iHLFI9GJRce30TEqnve5UgXY0v/ZOHQOyousbYG1ru4="}.FromBase64(),
ConstByteArray{"ZEBvUlKG8GtlzAx+GQ+CDZAqM9rBW90yP5sEmLfL6dg="}.FromBase64(),
ConstByteArray{"PmW3M3XxLEpWim3X9AcqvG9faXHtI8mMDTg2dWpssI4="}.FromBase64(),
ConstByteArray{"mQJhcPgC3qP7213OKoaR8pQZnis5pjDLVq5OemTvvJk="}.FromBase64(),
ConstByteArray{"21J+68lk0EA+VkKPMMFi2/9aUiLTOELkjqsFxLz3qjQ="}.FromBase64(),
ConstByteArray{"56ywWjqccobqxHFBTkbD46YJCNc4OZYj/QHJx2zTFAA="}.FromBase64(),
ConstByteArray{"T2bdbzg7CRyHw9oM3akj+inEMGUGNAQ2o5NOOXMZuDc="}.FromBase64(),
ConstByteArray{"dD+CoQu7Eir3iMm4PotzKdO5fECanVn/MG6/1PkksHw="}.FromBase64(),
ConstByteArray{"76Q8xSZpZIhYdgoLdfewDwVsazxrp9BZL0fO4jmva+o="}.FromBase64(),
ConstByteArray{"/KAXJ0c47KUTPehxdWAKwi5wqak5ak/iK3L4CJH8sDE="}.FromBase64(),
ConstByteArray{"7ckC5zED0LGbDzM26DwvOvjJYfTEDjb4CiR4/ReSkic="}.FromBase64(),
ConstByteArray{"kF6nuI9pQTti+YWJS1EAhGA4KdOD68b034tMEqDQ+8k="}.FromBase64(),
ConstByteArray{"LXwJFmxNDGGLDuUXSA8Q6S3XpqkpK3HhZugxlIkWVrQ="}.FromBase64(),
ConstByteArray{"JaO0rKo9ZUbUGjk8A8AgUSPax7nhln/1SG6/UuvXz/g="}.FromBase64(),
ConstByteArray{"XtLWjKPvI5BSFnzO8wkGsPZP4Pwv+/wjvq+1URfUTXY="}.FromBase64(),
ConstByteArray{"V2fbzGgOLVXTj2cfwNknGdzvfXH14vVg62iBkGcfBl8="}.FromBase64(),
ConstByteArray{"y/gfOXJWcTguw/mA3rOU7x6BZGoI+Jp4bcXXpi+TTYg="}.FromBase64(),
ConstByteArray{"Ml4UaMhDuA7tRZpTmhmxyXZwXrNZqJJIjLkK6y1hHFE="}.FromBase64(),
ConstByteArray{"WYArseBlXFCRZ39LJU/Jq3YScCpiva/xYOiIq3RQq3Q="}.FromBase64(),
ConstByteArray{"5moTQnpRBzn87bUp1l9X6GkdAyHKPgaS3ziHGsCzK38="}.FromBase64(),
ConstByteArray{"RV5BmLylQKVrD5zbNVtzcbRcGVvMaCKfb5RDDe5nPQQ="}.FromBase64(),
ConstByteArray{"aW8jWoANOiIx/P/GG8wmel9MYM4aaCAZS5CJ7RzHB0g="}.FromBase64(),
ConstByteArray{"GGLCVzBPPj+1BMfJ/2qn/N66eIkUlFoQBo/pAR17TbY="}.FromBase64(),
ConstByteArray{"UWcfExBglC9I6O68mMrvUc8JsORlikNpbw8Fqh2xFQ0="}.FromBase64(),
ConstByteArray{"f0M4smkF0xOBNSYBWzZd1iF7lWwp2oxnAUmNKGmZAyM="}.FromBase64(),
ConstByteArray{"EYKQyQAUUtJqT2uzxT6capUvwXxkEZmxzXAdCaj6Lc8="}.FromBase64(),
ConstByteArray{"DaPiYCalp/CXPEGlMe5TLZa+xixzmE2YsZJa+NjxpYk="}.FromBase64(),
ConstByteArray{"G4RUkBkI3Im4fE7trrYQKNsHbOep3XEuB572ftU2sLk="}.FromBase64(),
ConstByteArray{"9t4eFIiDexcwY+hikiRvFtC0Gjbv+SSlIf4blmVUsUs="}.FromBase64(),
ConstByteArray{"Bu29CR5SJ1IQ9f8CAAXjpIUBuddjGJy+yYz8ydjeaMA="}.FromBase64(),
ConstByteArray{"CI9SPZWJUw7j1pkL5G7cOnpC+2xSOVaen7fNgCrqXe4="}.FromBase64(),
ConstByteArray{"DXU2sXHeL2ddLYQ9pDmfOIVfhmlEvQK9n8gEfludZVE="}.FromBase64(),
ConstByteArray{"ctRh00yvVMGtZpGmo7CzCrtNrYzskvULHW4uHZhkj1Q="}.FromBase64(),
ConstByteArray{"TPmrI1IH8AZsxgKNK5M7aA+zO3eqTfzApFnNfqSrqx0="}.FromBase64(),
ConstByteArray{"5pYruufzrxXjoxfPzuV292r1LJthOkP5F6r2PWkqcM4="}.FromBase64(),
ConstByteArray{"LM+JykkgFeAxA8s+///86apj1tuFd3y7glMaT5d/cxA="}.FromBase64(),
ConstByteArray{"R7akWvcjTE9x3Enmb3LPzm5gLULkqGhzzightDG/MZY="}.FromBase64(),
ConstByteArray{"FulUtKeKu4fuWR9aiZ2ngjKPXS7VLHoYjf7QEhmmWCw="}.FromBase64(),
ConstByteArray{"rzlBEnfCiogEO8pukNqouUob7NMVAwacf2dKW6jCyyM="}.FromBase64(),
ConstByteArray{"pazgHk9p5EtQjYAFCO1d7o509+xL1/AcdZNMY02mrQY="}.FromBase64(),
ConstByteArray{"ykD03wsaS3ryC1cG0fVDbkCu5LaTNDLhMdjCtMDcMu8="}.FromBase64(),
ConstByteArray{"yaTKr9+N3prC+UF4aj74a5EOMimzGACCp+Rq87Lxxn0="}.FromBase64(),
ConstByteArray{"NAyOTA1rSbTb1lkTAP1JFvSoVrYefR2kCTjH/rAEfWM="}.FromBase64(),
ConstByteArray{"9J/NzSfw5DXPST4X3YM1v7jZN0Kutu/g5BPX++0n9zc="}.FromBase64(),
ConstByteArray{"rtviH07E0Sa64IA7vW3V6UoPtk6zsuPpRH1B0gV6ot4="}.FromBase64(),
ConstByteArray{"+CSdHinSuvMjPoH4cavGjuDs7307UnABEaJ7ZuxPK0s="}.FromBase64(),
ConstByteArray{"BdMfW4Tii4uHrCDNgQoLdeVfeet28ZuxfypcWsWxlEQ="}.FromBase64(),
ConstByteArray{"2MWESAnoFV4LRniUP9cVKoG0MRbIFhI8ZvfhsAdXJIk="}.FromBase64(),
ConstByteArray{"CHUY0cYpgu19jIfXV/YrZhG4zAk4L5IfwbjsX1/OxNY="}.FromBase64(),
ConstByteArray{"3fxM3SBagvimm/6IyGAgTzXY9p28TLsL8Fm4HRzn3zM="}.FromBase64(),
ConstByteArray{"WPZlUMdbLJqVCsdflcoPeJ169D/K2kBOmXlWHQa5few="}.FromBase64(),
ConstByteArray{"Xw8V16Ou+YrAbo0LzVhxavcDn5QIC81XHIcJXHi8ttg="}.FromBase64(),
ConstByteArray{"OYtXWe1Ntp9kNNZzoj/dS5waxT6/GtM8N2zzFiv72oA="}.FromBase64(),
ConstByteArray{"Q3bWIsFPCa9CYmoLZTIGsFgxAHLYyVQ4lszd3Mdbrrk="}.FromBase64(),
ConstByteArray{"dGJK7Pj2P+cGDeNEuq/o2jvN4hgoUnP3O7SVCfeBOtE="}.FromBase64(),
ConstByteArray{"HFoFeXdws7q01Y8FcJthqM1124UZsRTH/T8+wtM+PiA="}.FromBase64(),
ConstByteArray{"TSv1UYYwpjrU0hUy6OOfP3m4fwp1EdWBXFT0IZPkrao="}.FromBase64(),
ConstByteArray{"31c4SKsDFI6UwlXdsSNNF6+Q2BGJVmkrIXt9JqvDKBA="}.FromBase64(),
ConstByteArray{"coraSsHvVrR/CbCEo0t0BpRxh0n1Z22w0MdCsT7ZtBA="}.FromBase64(),
ConstByteArray{"F6ZC+woMhl72Ox6ZE6liz/LMSKPFQfZeIqL1j9AfJ4w="}.FromBase64(),
ConstByteArray{"MiOgXO/CcvwSfZbWJKkrjKE/rDA0fk4Zbe4yr9ZZEcg="}.FromBase64(),
ConstByteArray{"OCF/x1O5E4zOrRO8o0b2KbSA32Oj6AQtjPnNdsW03OQ="}.FromBase64(),
ConstByteArray{"7fZwalL083R4YZFjOOUdxVMf3RQhpcQ45r49bcjvOQc="}.FromBase64(),
ConstByteArray{"7qDJWnLsHKUCNUIzwHDvYMjPy9ZkUKKiFmgk2Yws0oU="}.FromBase64(),
ConstByteArray{"crOVrQe7Vpb/WEVnULov6O8m+qCVLdUWFs0OWbN+tJk="}.FromBase64(),
ConstByteArray{"a1XFu6rkzqhobuSWrwlENsgqw+V7LYZnmdDba8+xIjg="}.FromBase64(),
ConstByteArray{"SdWEpzf39ak71VvfKfGkVl0PThLz+xo/Pgn00B3ZqDE="}.FromBase64(),
ConstByteArray{"/ySKj6Rtcf+1c0azCph5lRc/xLKIpKx5IxFmAWKc8IA="}.FromBase64(),
ConstByteArray{"9sJwnmCz7Z4FnerDiTFZ8Ys2TVepAbqkUfA5c4h62NM="}.FromBase64(),
ConstByteArray{"68Ua+ExENQdfKS76K6Qee3aGY3W2vGSULeVxD2y5GoQ="}.FromBase64(),
ConstByteArray{"9uxcoxRbaK/TZ4y5+FdZ0k0CRnNTC+5vUM+bN6wWLzg="}.FromBase64(),
ConstByteArray{"izseNY0h5M1BkP6kaGkjg10kAhr6BoQq8nUsaPAD6mY="}.FromBase64(),
ConstByteArray{"rMHUsagYEQ436KEoRPWFFx601mb/AUa5xwcbbRyzPMA="}.FromBase64(),
ConstByteArray{"0xgFZHaCiwxuNnABrkXQGEBDeDIcCrjAcozDMMIovrY="}.FromBase64(),
ConstByteArray{"EkYwGkR6ivpPZ57OZPT0TTr2Su60tIILNNvgvJPkgZI="}.FromBase64(),
ConstByteArray{"yeBQY8xRhVPR5kh3cFw2zfLUqACJgekaLCRZt6nN1qw="}.FromBase64(),
ConstByteArray{"N6TDdaxv2uOnQlfDgEBxxAzvpcf0iiO3/yaCxTz0I+c="}.FromBase64(),
ConstByteArray{"53EX1ciTONKV1ziH3F4SiLrhg0F74tF/VCZ1DYdktg=="}.FromBase64(),
ConstByteArray{"4TU+V0O9436NBmbmslhepyyeu3cynEtbV2vWBHJ53lc="}.FromBase64(),
ConstByteArray{"nt2TaRYe9ind9Ony5+8o7lL8fIrPxtfvr7GXOeD1jvo="}.FromBase64(),
ConstByteArray{"QLwoY6wBw8Se1M4z2TUq+QvCr5idYvNfgpPKI4IU8LU="}.FromBase64(),
ConstByteArray{"yIMEkZTM+MROrimq8ronJIhPaNexywatswGrNpp+50M="}.FromBase64(),
ConstByteArray{"RLFYjqYPexo5NGXYBmdUXhH9MKsQ6oGhWjoNBjyxKOY="}.FromBase64(),
ConstByteArray{"pPQpGhbKfiIu6tdbBHq/DW69MW8x5A/K2XTwQtfXh1Y="}.FromBase64(),
ConstByteArray{"y+FJs0dS5fvfrRV8YNLKkk/Di1KAK4xFsJScfYyQ1IU="}.FromBase64(),
ConstByteArray{"yOvS5pJ9mb7TgSoZgTJuu37St2y3r0p1oFEWxA6lKFo="}.FromBase64(),
ConstByteArray{"/xRSutIWpgXTGxON0FUQb7XFEKriEbKNPw+FS/2w7bQ="}.FromBase64(),
ConstByteArray{"tniV/XB4LgUNJHJPauUVEqWhEX422H53oR4x+97t+IE="}.FromBase64(),
ConstByteArray{"43AGPzYMcYobb9LuqmZUFi0jR9COFUbCL/sIQyDyF1M="}.FromBase64(),
ConstByteArray{"OaiWMQ3G01YyM5muEyUA27+H2Q6CYzuHgr5U+IJxcZs="}.FromBase64(),
ConstByteArray{"0YO8qYRVvBf8pSnL3PNBuAhLniT9h76KkSqJsciRc6g="}.FromBase64(),
ConstByteArray{"22VJ3inJQ+e0Ngl5vVH751i3OocergmxTPB7ct6WFs4="}.FromBase64(),
ConstByteArray{"UQz95np2zohqB/OoIW3GisXTI1MoOn0ShwcERNKvqpE="}.FromBase64(),
ConstByteArray{"Zr416aLU98P4Tlh2Q/c5BBBd3wIGtLGjIaIevRd8jS4="}.FromBase64(),
ConstByteArray{"6pf+N8QKc9YOIValbrZbTx4UwXn35GROMvBLa18YHr4="}.FromBase64(),
ConstByteArray{"duOD5HtyO3Z4SnJBED5brl9vlANHVjt7aC0X7EJtKwo="}.FromBase64(),
ConstByteArray{"49/KFShxGWrZa/Kc4kQ4uBZBLMjijeICbmWn6EIYnXE="}.FromBase64(),
ConstByteArray{"9+2S68vy+DdBs4qv92x+1X2/kfWAR1gXvu9iBxZvP1E="}.FromBase64(),
ConstByteArray{"YYs6A5tCEgRaKUY8evb0eTP71xrp1L2t1u715iM6SXQ="}.FromBase64(),
ConstByteArray{"bgtgwUZOHBkMslTgazKV8Pccw4mIzPl1h35YvP7ZTVY="}.FromBase64(),
ConstByteArray{"Z/vx9OhKKadK9JzfDytJ1XXiVGC5evplBmMe2qUKKm0="}.FromBase64(),
ConstByteArray{"xb2q/Bl3litRmONk+c9KplzbcC2RLbJ2TYQ/+fNgnrQ="}.FromBase64(),
ConstByteArray{"+WYdHi2X44SE9sUqkgDowMqzzJMquqntw3EysKs3nSI="}.FromBase64(),
ConstByteArray{"/y+bCWPE9ex+V1kMLCf8oq8lgcMl+dBbKT9hX2OwlrQ="}.FromBase64(),
ConstByteArray{"9TjCJO9/efSJgEj/jFBt/4xC/gOZoThMvMzKDHH08Yw="}.FromBase64(),
ConstByteArray{"dHfJtSoJfsB5bg3nryTHJ7L9C44DBihy7Xn3+EExUbo="}.FromBase64(),
ConstByteArray{"SGQVt4u3vcVJQyyM33PpNWc39dhaKJQaJCV9rxM/CxM="}.FromBase64(),
ConstByteArray{"N3OVljt05RQUyaMc/pxIRcqHvyaLdnRpT6FIVL9PY8Q="}.FromBase64(),
ConstByteArray{"E1/cWAAGPVvOyRN/j1szg8/wU4D78ErHo2AOTTcY+2c="}.FromBase64(),
ConstByteArray{"uJ2UflnrhDDp6m1Bza1TJQs4Ir8GFazwKfO3I4+BYvY="}.FromBase64(),
ConstByteArray{"MWbohfZhDYvjHFJdocjHUhdlAJFi11pc93MEDAkr20k="}.FromBase64(),
ConstByteArray{"wAlaNZcfVc3AUw6dtrzLmcworofhz3jhj0W/BQ8EmpU="}.FromBase64(),
ConstByteArray{"7t+tnMNY2f3eDNO0wma6NU2H0+zmSrgfzvD24FAX/1E="}.FromBase64(),
ConstByteArray{"Q+dU75JM6v36pzK3U3hc+TN8DL7JKqRamkg679CNTRw="}.FromBase64(),
ConstByteArray{"xwqI1X3M1Gq92egSwVpJ27E6Gs4sy2nK3ZkVLbQ97Eg="}.FromBase64(),
ConstByteArray{"33rdIC9wLOPzXKrsETOOGOPDc6G4LluJ7abFlg8gXMU="}.FromBase64(),
ConstByteArray{"xccyM0OVgdcb3o3DuCCxV7XPWXN+KFeHR2KO4oIZdWs="}.FromBase64(),
ConstByteArray{"odVY7PizVBPbj7aTwXjaeYaJcSLA0t/PUW4aknIeavQ="}.FromBase64(),
ConstByteArray{"BssALxM15st68wgDf1ciwBCgtg9GW5BwQANj/Ud4Elc="}.FromBase64(),
ConstByteArray{"U0BYxCSt2n0wAeKGpQ8SIGWxh0xJ/OdxqZpb3YDBqN4="}.FromBase64(),
ConstByteArray{"UxvLL4lSJjbgUZe8PH0gfQhMpw1DdI43gesffjV67EI="}.FromBase64(),
ConstByteArray{"eqodl5AycIqwOUr6KwDhI4YGm949/Y9TNLIg3lV/H00="}.FromBase64(),
ConstByteArray{"4V8peqnmGAByULUodnEuSEQdKuHRsLpzucFDkbVFWME="}.FromBase64(),
ConstByteArray{"X9ZlpvZivDJDII6PbkeKGSpVBkK56fsj8W0iY0uY+N8="}.FromBase64(),
ConstByteArray{"Me3DbRTqVy93ewCOEs1MkUMEekGHdPAbDD2BSjQTRT4="}.FromBase64(),
ConstByteArray{"VLvHm9wQyobNW1SQhDWPLAtaxaGH153L+TZP9EqDAgs="}.FromBase64(),
ConstByteArray{"oU3tkjOs/uwgWbiazNIQXF99pPEaJW52RBUouilLoA8="}.FromBase64(),
ConstByteArray{"9YRiRQJOYjv25XdKmZEStdI8vGyoowFH08TkoFSGeB4="}.FromBase64(),
ConstByteArray{"ZH2cdH/y8eUgN4T4zQAPlGYPDY/l/jxat5oAIRe42DE="}.FromBase64(),
ConstByteArray{"Y3bxk9FsulT4ooaPG6kI7pKYW566y3GsFU27diq1AbU="}.FromBase64(),
ConstByteArray{"y+pcBL3Yzpx2GtGV4VBno0rIROUYsj3+UwrlDt7PL1I="}.FromBase64()};

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
    ECDSASigner signer{all_private[0]};

    auto const private_key_data = signer.private_key();

    output_file.write(private_key_data.char_pointer(),
                      static_cast<std::streamsize>(private_key_data.size()));
  }

  // Now use these to create TXs that are entirely within 1 lane

  std::cout << "Generating TXs: " << count << std::endl;

  uint32_t lane = 0;
  uint32_t total_generated = 0;
  std::vector<ConstByteArray> transactions;

  while(total_generated < count)
  {
    ECDSASigner signer{all_private[lane]};

    if(lane != 79 && lane != 202)
    {
      // build the transaction
      auto const tx = TransactionBuilder()
                          .From(Address{signer.identity()})
                          .ValidUntil(500)
                          .ChargeRate(1)
                          .ChargeLimit(5)
                          .Counter(total_generated + 8000)
                          .Transfer(Address{origin_addresses[lane]->identity()}, 1)
                          .Signer(signer.identity())
                          .Seal()
                          .Sign(signer)
                          .Build();

      // serialise the transaction
      TransactionSerializer serializer{};
      serializer << *tx;

      transactions.emplace_back(serializer.data());
    }

    total_generated++;
    lane = (lane + 1) % 256;
  }

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

//#endif

  //if (argc != 4)
  //{
  //  std::cerr << "Usage: " << argv[0] << "<count> <filename> <metapath>" << std::endl;
  //  return 1;
  //}

  //auto const        count       = static_cast<std::size_t>(atoi(argv[1]));
  //std::string const output_path = argv[2];
  //std::string const meta_path   = argv[3];
  //std::size_t const num_signers =
  //    (count / 10u) + 2u;  // 10% of count with minimum of 2 signers (src and dest)

  //auto const signers    = GenerateSigners(num_signers);
  //auto const addresses  = GenerateAddresses(signers);
  //auto const encoded_tx = GenerateTransactions(count, signers, addresses);

  //std::cout << "Reference Address: " << addresses.at(0)->display() << std::endl;

  //std::cout << "Generating bitstream..." << std::endl;
  //LargeObjectSerializeHelper helper{};
  //helper << encoded_tx;
  //std::cout << "Generating bitstream...complete" << std::endl;

  //// verify
  //std::vector<ConstByteArray> verified{};
  //LargeObjectSerializeHelper  helper2{helper.data()};
  //helper2 >> verified;

  //std::cout << "Count: " << verified.size() << std::endl;

  //std::cout << "Writing to disk ..." << std::endl;

  //// write out the binary file
  //std::ofstream stream(output_path.c_str(), std::ios::out | std::ios::binary);
  //stream.write(helper.data().char_pointer(), static_cast<std::streamsize>(helper.data().size()));

  //std::cout << "Writing to disk ... complete" << std::endl;

  //std::ofstream stream2(meta_path.c_str());
  //stream2 << addresses.at(0)->display() << std::endl;

  return 0;
}
