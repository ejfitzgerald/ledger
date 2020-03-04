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
#include "chain/transaction_layout.hpp"

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
ConstByteArray{"ASdQr5InsPXEAwFZFvl4d+kVDVTr5wH7tPff5AQlq9o="}.FromBase64(),
ConstByteArray{"kPbn18UcXu+oRlvIhAW+pYANlRYPmURgytW/WjHfcVA="}.FromBase64(),
ConstByteArray{"A+s/EZnxfuBLyDchOfpV5ihMFn8fq3LGQtebCugh3+o="}.FromBase64(),
ConstByteArray{"D0ygc7IBtKxVCcKzwgPMPoqbNLHdiYEhR7QZA83Zu80="}.FromBase64(),
ConstByteArray{"v0FOhs4WId0djF958C0BTE7S0c3fol7T2gnkTljLSWQ="}.FromBase64(),
ConstByteArray{"hK2UBS/Gp/zRt4OLsezSSmfLGQ8LkbU5IZRFUq3tpRs="}.FromBase64(),
ConstByteArray{"02v6rNYash/9P/yIChOoxBDlzlDOsxiN9sewwGH4FTA="}.FromBase64(),
ConstByteArray{"xpvhvzrkiFi3iPHio3KMY5lggc7nvC6aBNlI2K5cWBo="}.FromBase64(),
ConstByteArray{"StNybwHVRYmPvwRjPEWloy3a4NirhNhWMAG19+PlryI="}.FromBase64(),
ConstByteArray{"DwPNFv/a/ehRjBnWRBiatPZmtJLBg38cBLJ3GrzJDNI="}.FromBase64(),
ConstByteArray{"knqd7FQECg1eSmLHEJHgkaOXsCFp7YdBtFSaQPOyfOE="}.FromBase64(),
ConstByteArray{"eS4X5iBx8jiB2+rrFYC6Guy7oQxGAOZnkAYBLS/3XXM="}.FromBase64(),
ConstByteArray{"a00KDuIfPlgJFvnnGtZrOJh5XcXd6v5kqseuN/2ZQbQ="}.FromBase64(),
ConstByteArray{"nd79xkiECUL/NgKBfpeLrKCkISOMVtJcPe6k3AV2uYw="}.FromBase64(),
ConstByteArray{"X6jEMxc5oey3Q8Zw2byzfihc1s+4xnP0ymskkC3bKEU="}.FromBase64(),
ConstByteArray{"ycwY7fnobUE9OI6g8dI674rOpvrjskAbo2umMFfmUfw="}.FromBase64(),
ConstByteArray{"AhCw3quUTf28LQFPjmCVa5AJfFgPIHEHXQ1b5fQVmtg="}.FromBase64(),
ConstByteArray{"LsP07NeU8Q6/PZjzqjbEJ9UqKKNBM2zXkGFGa1GE7ow="}.FromBase64(),
ConstByteArray{"w702HpEfbZz9gTK+9N2cn2q0qJu7l27P1w+I63LRpkU="}.FromBase64(),
ConstByteArray{"Ny5LfoqDZHYlZKc+BO4ExPnzIZtCYrJJyc8KULNgnB0="}.FromBase64(),
ConstByteArray{"EEbBgph7OZx1zbjEAG9dgLS5Ub/QPEFr0hwVsrlULjU="}.FromBase64(),
ConstByteArray{"kUMn4ZoCSJisDCtcJyR9fZ14Gz8HgloGd848BNtDgus="}.FromBase64(),
ConstByteArray{"aVWIskr7hxwDwAcDuXyGYs1Y2QwwpFMrTsktYCUBwQk="}.FromBase64(),
ConstByteArray{"X+4/0vmbU442ekXFzORj+TXoJumKnUdbaU0rG2zxXm8="}.FromBase64(),
ConstByteArray{"JMDZtHIlxAcw7nTPR1Q2GDL//FKKI0DGGAnVrlHW3vk="}.FromBase64(),
ConstByteArray{"/4GeuxRLzYWjTUfuO+7RuRtI7gMBYHzNYI2TcEA70gM="}.FromBase64(),
ConstByteArray{"7JeQ4TQC0r0RDdn9hYTM0bD0VHtjsh91q1fYmwwGhqg="}.FromBase64(),
ConstByteArray{"MZCs7b6ggb8JO5JUzXh8o4c/Y7/jdPgFF0zRQkzgOJg="}.FromBase64(),
ConstByteArray{"68lAFQztfw57bQlVR4Ng5cgdbfWyDFQUdb582iD76Lg="}.FromBase64(),
ConstByteArray{"XB//KCJg4C2kjFNmcsuOXIQoUC2LHHWkP3DAx++eA8U="}.FromBase64(),
ConstByteArray{"laBfZS2aihgiJk5+Uc0Ju+iTs3KRJd4NRkYsdx6XEmc="}.FromBase64(),
ConstByteArray{"5DE0eOtCZpdCQX/dj6bSPxZx4zVXyX+n7WlYTZjp97E="}.FromBase64(),
ConstByteArray{"Egiht4y7MCn9OEDAmlOl4RyOOJUpUAtsg//6qhKD+2I="}.FromBase64(),
ConstByteArray{"Z7iU02wZ2xahvWn1UKEK8N05BgmJK6YXD1voT15LCgs="}.FromBase64(),
ConstByteArray{"lspo+JmMwJZo7hzDJO6S20qV8xlIb3HBerTxT0tn2/I="}.FromBase64(),
ConstByteArray{"vTtu03z0Ogn/Aa/dLv4il1vIebb39ay+6ZOXgwYCfUs="}.FromBase64(),
ConstByteArray{"ufLK3VmUpeX6MC9xNXz/6usdSc8YBdP385wpZuGVyC8="}.FromBase64(),
ConstByteArray{"jQ7muiuAf32j0UJdlkQ91wlteuzcaixV0dG4JH5wov0="}.FromBase64(),
ConstByteArray{"0ISmnodVpPwE9G7xhakHOeZ2lAB+hZEnZ8cIQBrzeRY="}.FromBase64(),
ConstByteArray{"wtFLy7aHv7is06oBfVeEiStL9I+gnrd+dpSm7VG10+c="}.FromBase64(),
ConstByteArray{"MMAp2UoctEyTvyvQN6rwyqXKtLkQYREXBJmRL9Kg9nA="}.FromBase64(),
ConstByteArray{"CA/MR0o4t+UnnE32oSZq3z2tKZRrm1KelIXUxsjtuAg="}.FromBase64(),
ConstByteArray{"Vvmc25YN1AynKiR/uuWwmvMtkZ7qdt1Udge48hevMC0="}.FromBase64(),
ConstByteArray{"loOKqxXUh8CCOoIJ4eRbJqsztzubshZECmIS0PtQmDU="}.FromBase64(),
ConstByteArray{"62SagDPlfhdmzNZjBb+Ytlz+//1wWmS8id+Ch2Iljrg="}.FromBase64(),
ConstByteArray{"vAkgvhJG28OA3/bSVKio9GAyzMeDiQlh9VM2RbFh9nQ="}.FromBase64(),
ConstByteArray{"JMbBkrdlf8BeZwGsUy+abM71wsM1JJUpC3hTSVlRKXA="}.FromBase64(),
ConstByteArray{"nS/0+vgLskkZEyLrQjVyyrOUgcRAgIPi1izT4nQI4VI="}.FromBase64(),
ConstByteArray{"H85H8VEMFS5zEr/MD74mh4aEYw14ETDyEesIr//CGkI="}.FromBase64(),
ConstByteArray{"4vF0DnpqiJXBg1ItmW0I0ApURt1XyFX4/fQZ2ZMO/iY="}.FromBase64(),
ConstByteArray{"emGl4sUOkMULwmJBedUJuIQqwMCvcXzJF871uNGGJhY="}.FromBase64(),
ConstByteArray{"Y2O0bPHy3SBssTDbRtuP0F4FN7givZrYlKsxxzZHOk4="}.FromBase64(),
ConstByteArray{"83HzQogJlfiqqrStZBODg1LDJ1H/Cxg/jFmLf9V8fVQ="}.FromBase64(),
ConstByteArray{"r67mh2aW9/xGfJ4mRQkcd80VOUbGGJeQTfxvHP+FSMo="}.FromBase64(),
ConstByteArray{"jVkx2NWeQdgd44b3tNAP3WXoOLqENgEKMF4Des4nw9M="}.FromBase64(),
ConstByteArray{"AOae57k8nGBU21cUsq9a7jQ3YT6iKzGmDuveDfL9dBU="}.FromBase64(),
ConstByteArray{"xXvSwz+oyzFiTv2EYInADmZFQp7OYV1cAX5DsOL8Rvg="}.FromBase64(),
ConstByteArray{"VQfQ+u6hCye6ufci3tuivs3V10GwXnNl4q9pSK9o62Q="}.FromBase64(),
ConstByteArray{"uxWzqjvixSnFDPWyalaZ1nK6gzEG699zydvWIxZANLo="}.FromBase64(),
ConstByteArray{"fGDeHjSp425mgRmIGw6XbTaGeUvf/uGOYb4cKOecPL8="}.FromBase64(),
ConstByteArray{"UCz7eapVIeZfFjRUMXxdLtKDe7kxo+0tffXg6xRhiR4="}.FromBase64(),
ConstByteArray{"nI1Hkl1VcxNqX48RtmHEg/c+6oarB9IlvpS7V2q8uqY="}.FromBase64(),
ConstByteArray{"wRGvSGNwBITHTyKPplg2SxoPqXgRd40j/8hycqF9cV0="}.FromBase64(),
ConstByteArray{"DTjgdjmqrQTCR/I1v4cSHDqjQxGhW0zC1l48E7wEmOQ="}.FromBase64(),
ConstByteArray{"4oAOqPeP/C8QxOrknf3vI/d/1++kkox1uYw2PFzFDjw="}.FromBase64(),
ConstByteArray{"DXfnwbf0Tn26zPa28SU0Fu4MFGd34+ChBaKNLaQDPn0="}.FromBase64(),
ConstByteArray{"WSrp8Jm+C54/GD7iNMmgRkPVncdzPi+9B1Iznxi1Wmo="}.FromBase64(),
ConstByteArray{"NiIK0s6Ex/a2m6l55E4zN6dEQuw+256rMNab0yet5n0="}.FromBase64(),
ConstByteArray{"C276rhBVHKxMFptBiETshSn0rXuRi2IXluhYlEKsEm0="}.FromBase64(),
ConstByteArray{"IYdGuxZW+but03zoIsbx/bdY1s/HiLPI1CalUmgGJD4="}.FromBase64(),
ConstByteArray{"87VW3vZUDVmGfivF7BOw/9C3nzhk6b9Y/sMAehRXDXw="}.FromBase64(),
ConstByteArray{"6pCCydvyM2ndq+rYDufQW8xIYrbGXQ0ODSTpM6+979I="}.FromBase64(),
ConstByteArray{"ndIb9gtmIwIVr6y+6agUAXU1GAOOAZDc8u0pDZMOeCY="}.FromBase64(),
ConstByteArray{"ZEX9ODyzE4M/2AdbI9xZV67EZHhEhyoYUR599Ziti00="}.FromBase64(),
ConstByteArray{"cKGxAApAjwSF652WXPzulrQ/iTT8NDA/wFjUCJSSxNs="}.FromBase64(),
ConstByteArray{"SI0fRZyQGhfBinKe96RA+wfRLORjpmVXwmzgXEIwSFg="}.FromBase64(),
ConstByteArray{"LOOwKZ5dw0Wcj+cuxbfD9O+OAzBbAg/GFejuqwVd3NE="}.FromBase64(),
ConstByteArray{"Ou8XBYfinoSHbfxcHcgBEwFD/a9Z2/ylAJWBNJ7WLTI="}.FromBase64(),
ConstByteArray{"m63tqtWWytfBwt9jcevKk+1hCJXofv2OaVf8upr5R34="}.FromBase64(),
ConstByteArray{"anNDxV2GlUFafp7v4dq+SvtcEPLaHOZNXGQ/TLUFxXs="}.FromBase64(),
ConstByteArray{"6q3BzvweYfkfIyVqESq1JTQyH7BD33eELjkqlKC1U5M="}.FromBase64(),
ConstByteArray{"NZ+xZEaiFrXdig+dvuOLFgflfptiBX4rlIdWoEMkDDg="}.FromBase64(),
ConstByteArray{"pYYXV/0U1JBIBKoXdAn6zCIqjn6WoRjBgmWcpXeIH1g="}.FromBase64(),
ConstByteArray{"Vn3MSoIy3JtHB7Dpad9yBCfNsDLRQd17Yq/OZ1grcTw="}.FromBase64(),
ConstByteArray{"y6CCXJHvM5fBwzTvk0TEmavf1aHUbGjlHiD4B0xvDD4="}.FromBase64(),
ConstByteArray{"+eFxbYOQDVznkvZaNEW+XoSL8X6FS5QMDvrzp+EjiEI="}.FromBase64(),
ConstByteArray{"iLmRM836obuSGnPRDAa4H5XcfuJGFhUNdt6GzRmA5Ng="}.FromBase64(),
ConstByteArray{"xxuobAcayNhxWVQT+Wr72p9hebsj4yweRlHaru9zUJY="}.FromBase64(),
ConstByteArray{"StcmdZQ6tIO/NrWHUxejVi0lJkIpMEidy2u2F5fKD5M="}.FromBase64(),
ConstByteArray{"3p6inxGytjOaBzn1INRUHwxJl+mwf+cd03MINNpXEu4="}.FromBase64(),
ConstByteArray{"stLxnggmLpaPqKpIEIwlevcxoEzAKveAkpvccuYhtrQ="}.FromBase64(),
ConstByteArray{"Fc8blFQR+WrnxGxuQpDr6LNWVro6IdlBUBawMD5nxRE="}.FromBase64(),
ConstByteArray{"TxFaP7BIT2XcsiCwSGXFLeqnqmvfgIX2rsrnKRIKbc8="}.FromBase64(),
ConstByteArray{"eeBtAsO+cx8OYKHSg4UTJAlwrFn2s7IH2DEOh4byHyg="}.FromBase64(),
ConstByteArray{"ao4tCB+c5g7aYzgFoKObEDlXP1444et447mEt8Rnomg="}.FromBase64(),
ConstByteArray{"4F+GPOQ1yEP27oyP3rw2HfMRNa5j499iv2IkOtboipE="}.FromBase64(),
ConstByteArray{"4jw/wWmXrb2oaPey6I685LPwhIC2YiwCUntD64BqV/g="}.FromBase64(),
ConstByteArray{"XL8HLvLUVQVCN5xQYI+mjMt/aBU/MVb0XnF+UhNY124="}.FromBase64(),
ConstByteArray{"SMHYJN7cMkpdRi6HI7aIHTWYuLfho5bi/Xmu/o8TRFE="}.FromBase64(),
ConstByteArray{"s5d09MEBi90OQb4b71fAynG0GQaLOa7K3m2mbi0XXQE="}.FromBase64(),
ConstByteArray{"fcGTuDWu8pSTjVyFiI8XZ/q0/Em680z3loBJNI4v1uE="}.FromBase64(),
ConstByteArray{"MpRXNpyPIDhbf0yagBdOcfzmm9DC5ZdVhKUjP6kEDNI="}.FromBase64(),
ConstByteArray{"IJE5Dxpe9by7uVdFz6Klkf6qOYJZSWd4xxWmJflQpr4="}.FromBase64(),
ConstByteArray{"XjI+k3SjEukijETJQW58kcPox1mF3O5THC9PIa8+9yI="}.FromBase64(),
ConstByteArray{"T8BcP8JjfUEdML7G9D78AyYhLrFrU+ITkbdkzE/iJBA="}.FromBase64(),
ConstByteArray{"Tc3P4oMK5tPm6qJ8oNqtzsZ7dQEHmiKIcPdPalR5uxI="}.FromBase64(),
ConstByteArray{"lDMfvbGJ4DaSJkJFFx9B7Vt2STUQpQZg9iO8fUwWkwM="}.FromBase64(),
ConstByteArray{"fnGnZb/upKD1/u0KjVvtkju2UFJFfYKq1gBbR0cpQAw="}.FromBase64(),
ConstByteArray{"SiGUK8gQbij56IkIPh5YXDgTWS0mNEjxzei4YrDcNCI="}.FromBase64(),
ConstByteArray{"YR+ZMeKa5E88lRvMGseutaJzSt9rXw4fuUwoEmi9fb0="}.FromBase64(),
ConstByteArray{"KAnnP4LAluSSD92GTnggTXNNzqAVXHrjBCXdTEXbI5E="}.FromBase64(),
ConstByteArray{"E6wn5Y8iP9QC+0nhAAeey56chu2NYYo0Ibi0JXgp/X4="}.FromBase64(),
ConstByteArray{"Y/oGupOjaJr1LVMlrQ3Wymv+VFoBxky9eIkDUV5tYUs="}.FromBase64(),
ConstByteArray{"TuSytr/KDzYevlG25Y9RQi0rNGF602fIxDt3/9ViIeE="}.FromBase64(),
ConstByteArray{"aaTn4kLy0070yNiakybqI5sme1jXKZXKcfYYI7nTnHs="}.FromBase64(),
ConstByteArray{"z1Y2MHbEr5FVLx8Hwt9n/XV9obpvUnqWRi5sSFrjgGs="}.FromBase64(),
ConstByteArray{"5wv+sCIuy82ngzzW90SkrJrM7LyySwQEGir924/TE2o="}.FromBase64(),
ConstByteArray{"VIIyGG7E2KJ8DEpPM2M/JjX2iuMW+0OboAQ+xPmuJi8="}.FromBase64(),
ConstByteArray{"XJMfj+YjYoR2HAJmm/+5xII8+yI824hbqmDnRITDeWA="}.FromBase64(),
ConstByteArray{"eTgmL2+Ty0yHzPtqHbrdQ9A8vgfIbYw4qb79+jMsL+E="}.FromBase64(),
ConstByteArray{"0jks5AnYwJ5EJkGVfIcJPw+2WXLP8a+5ExxEg6maF6c="}.FromBase64(),
ConstByteArray{"iElBTyx1XrEYN2XVO03L8+XFximvgJZUUC/T5ftobfM="}.FromBase64(),
ConstByteArray{"gbw/dY5jJnrhTkE09c/5fcEjWOpaco5lu5vs9EWUVrc="}.FromBase64(),
ConstByteArray{"okmKyrvucPhFJ7OA2Sq6IokbwTV2AUHReSqOfZvVgrM="}.FromBase64(),
ConstByteArray{"DOxm4Z0z7/YfhRFkT9KnhJDG2Qkxil83RJyEz1tNqzU="}.FromBase64(),
ConstByteArray{"aI+Vj/Lt3EwswfLRrDSfc2Nn+G4z4wocR013wJAC+8o="}.FromBase64(),
ConstByteArray{"xloI1kxqMUHLossSfgsIUv4JAdwqN1KqnIRQNxrQlWc="}.FromBase64(),
ConstByteArray{"Md9YhAMeCqlF/aX9bAIF+7PuBQFKgnK6tAI6b7YNchQ="}.FromBase64(),
ConstByteArray{"fB5CaAgwyuNRFCwLGe3Ye1Dz3LgunwZnv7+WdI6+yo4="}.FromBase64(),
ConstByteArray{"iqiPEc3lrdD+txcf4pj+Gmud0xbmVCyoUmICCv3gcj4="}.FromBase64(),
ConstByteArray{"SCB25nGMHrxGmAhpCZy+iZyBibz5AcSh5tG7b9FXs38="}.FromBase64(),
ConstByteArray{"2/bOcA/aXajRnu1gPHbtv7q78wPOk0tlDAQGvS+Dyjk="}.FromBase64(),
ConstByteArray{"5qJ5X4EiQhin6OutwAF9li/uBSUDIQYo3egTJdl6zJc="}.FromBase64(),
ConstByteArray{"AymNxM64jYS4aGj1XBvoWKPkgALy1lncZVNZUGHLxYw="}.FromBase64(),
ConstByteArray{"AP0rFrBgicD9exa/Y4DOaHq+YEGb/MhyWVUwjgQSKmQ="}.FromBase64(),
ConstByteArray{"cMBodSlzb4wSwPGlAudqq3i5LW7fCUuNbBCXCW2mslY="}.FromBase64(),
ConstByteArray{"xm69nvc/+S/QBlFs9QzCxAcvQqSfRRMBn1XXpe+uX4U="}.FromBase64(),
ConstByteArray{"1GAeGh5X+mxhKOrx1p7YDdtqhW8JeeSk4Lr9mT+TqBs="}.FromBase64(),
ConstByteArray{"9exFHukOPvMBaRbS/tI/XgkoK4iC95yVYphU5hBhtlk="}.FromBase64(),
ConstByteArray{"H3l6e9fmNYV2t+BqMsuTENhbh0Qy4btHSqJuKl9rJW4="}.FromBase64(),
ConstByteArray{"Pio77i511A3f7JL1UNcTRy6b9AxiD/rP7xBiZpVXcYM="}.FromBase64(),
ConstByteArray{"N+lvlvJxDJkOqsJGxPaKTsA408tg6T8QE/9Xklo0mqs="}.FromBase64(),
ConstByteArray{"ITUzQ67/xj5WMTXre/0H6NxxhFxOAqZ6do5goUbFnro="}.FromBase64(),
ConstByteArray{"3a4RpHlC29/BBVrb/KPEnnIvwNXya7wA2Frul4wx7y8="}.FromBase64(),
ConstByteArray{"Q8Njsso7vyZyxlQekPcV1ejtM/6Jpz1BXyjb3dyZbm4="}.FromBase64(),
ConstByteArray{"Ud2dGkOLY9gC4qdCGWPgvg1DYfSxdTId/r1PQyKd9Qc="}.FromBase64(),
ConstByteArray{"77dk5dQUwGQzNjMIau5gtheSzJnx/cWfcAUipIHx4EQ="}.FromBase64(),
ConstByteArray{"SIxjGy0EH8L1rGft4exdboPWLQMzMgNAbVBHhGCIfxg="}.FromBase64(),
ConstByteArray{"SRMZCplFtFr1GaIuKQwzKHSGlUO4xy/jNJNQwjmqjB8="}.FromBase64(),
ConstByteArray{"h2sALwPQXgXD9MhXKYMwl7Swa32TahsF0WD0cv367wQ="}.FromBase64(),
ConstByteArray{"8Pv6CK9HgYC20aNz8HXZJ5vz/WPjtPF3UJAsbFXBNoI="}.FromBase64(),
ConstByteArray{"pkgU7c3VODBU5VAFTSDltrK62SxQ1QNEGcsJw3QnVNU="}.FromBase64(),
ConstByteArray{"nXi2R0Q94HZeretECASjchPHgBNUAkvXk++8qnPCsIM="}.FromBase64(),
ConstByteArray{"7nZuwn1ky61JtUrxWfOZKjUXwy6iSi0XQKMKAaT6c7g="}.FromBase64(),
ConstByteArray{"btAV8B5FaB2utA5fp3wG9bXcbH+4lpqUN4olVhNMi7E="}.FromBase64(),
ConstByteArray{"WpI3ApnM7a3Bwi9Vr5Wa7KK9LKQ3ZH4uPooZz4K/Uh0="}.FromBase64(),
ConstByteArray{"5K2ylbiUC1h0x0UHKMHncc8VFHGuQzFXam5z0jCQnPo="}.FromBase64(),
ConstByteArray{"nHpPfjXL/fNB9qF+769RkkWACD3E/plOJPOyVe27hIM="}.FromBase64(),
ConstByteArray{"1QB96YwLzfTse5TIJecT7tbyhYjsY9pwE7NPkW3xBok="}.FromBase64(),
ConstByteArray{"6/yzGC8uCq6j1uLgWku9CE4a0LardLTKLZykial1TIQ="}.FromBase64(),
ConstByteArray{"RGJjS1npDkpH5MV00oNv48J4S1gpKp5T2C3cW5KQ5SQ="}.FromBase64(),
ConstByteArray{"EnLjYcx80xm5F/zOB0FiM0S9QM/KXCldYOQ1TTReTWY="}.FromBase64(),
ConstByteArray{"W2TteiRtTEnj89pHFzPXKaqoG+Itk+BgRHFQnKTcXxk="}.FromBase64(),
ConstByteArray{"X9b0XpZMUXZE663LgTcvKcawua2J56kadNvJEEm4F4Q="}.FromBase64(),
ConstByteArray{"KA97UgYE19eaNk/hSgEfUs7FGXEuDI1lm2vFC46vipY="}.FromBase64(),
ConstByteArray{"LK+p315/7MissvutmuUCtSZhC/saGB9vokiJUbw+ytY="}.FromBase64(),
ConstByteArray{"pIwrVE37P+WFtLuTAeoPMJBZ+AyAXNVfFxYc+yq+FeY="}.FromBase64(),
ConstByteArray{"SbOFoDCMEYP7+zOF8HJRIwKfRc2H+uIxY8m6qo/5k60="}.FromBase64(),
ConstByteArray{"UkIBoV7AY3Pyp0fA+s19+ATIVBYp3Hi+pRmS3+k7A6w="}.FromBase64(),
ConstByteArray{"YM1L5yp4DGByurrz7jKvWqlXwyFalwsm/30iqRnin2E="}.FromBase64(),
ConstByteArray{"o04uHbdI3K2I+OoDYXGW0yFwbuVxvX50me3nXCXpmgM="}.FromBase64(),
ConstByteArray{"kFcL53pt/LX6kQzNuZm/IAgfGPIvfwvKpGlVElieX18="}.FromBase64(),
ConstByteArray{"VG9uZ+D6ppCK2sg6cpL/IMWsQaWlHaKPgo0pPuJjMbM="}.FromBase64(),
ConstByteArray{"E6xRzDvpC9WjMPSTwbsZRMoDP30//HokFlS4QaiDpFM="}.FromBase64(),
ConstByteArray{"8cwa2nlwa2MMjEgv5ouL9NXNN1Mp4GppZvRm64Jig68="}.FromBase64(),
ConstByteArray{"9ntegAiF9KHdSp+1q0Oyqw4ct9DGa96A8LdQ/lSOXys="}.FromBase64(),
ConstByteArray{"C2YJRc4obiIMlcWmDOdPLWIOj5MpDtfhd2dfVu/j+yc="}.FromBase64(),
ConstByteArray{"5g3QekMV97Mk28Q6+DabUL02USGEQrMaTZRoSkh4l3c="}.FromBase64(),
ConstByteArray{"K+vvCWpzA8jBU35Cuv5uAkReIm/q+lp8HKotXQ6LOrQ="}.FromBase64(),
ConstByteArray{"nC3ZqjRRY28MJLIx96mNwDuNmLUgp0N4TLUYVik0V94="}.FromBase64(),
ConstByteArray{"gkbkkE/4kDKhryf2BfIqLK/9vThb4ga6cmTqILtMgzY="}.FromBase64(),
ConstByteArray{"XaZMu1xaJx/gkbimzVmX2QbrLgYLY9NRtLawEvY/8Vk="}.FromBase64(),
ConstByteArray{"RW3I2q9PSbHxklFYS1blZpes4GNEjjs0Wla1O7IB90g="}.FromBase64(),
ConstByteArray{"dG4GJAS53WMmlwb0hjsSIVGb3zg0yiZwCgho5GxD2kU="}.FromBase64(),
ConstByteArray{"4KCXjMq5OcIlAivnUJamRksR/KXyFfTNUn3km0IudUo="}.FromBase64(),
ConstByteArray{"PT6Yfb3HVcRwxBvV34W3DUly8kq0LgMwkxeEwxE9o60="}.FromBase64(),
ConstByteArray{"3QSOgUoyogD8l53+eA0JmsGFPZe3EgMkDlKwrHYWUlw="}.FromBase64(),
ConstByteArray{"NIA3luvnXNp+I5tkGYx055NVVR7+nLulqFmeoX7KK1w="}.FromBase64(),
ConstByteArray{"lNZHXDTX7VnPdNoW+yXv1m2E/v5KxtblxO17arxJ5ZQ="}.FromBase64(),
ConstByteArray{"BIKN+bWe8YpZqtaRGdL1LuwLFbn/R0fiIn66kMj9vVM="}.FromBase64(),
ConstByteArray{"4yj7Z+6dn5N8H82oGFoaxhPPoAQHlJm7fT3x4xaESjk="}.FromBase64(),
ConstByteArray{"UW7cwBKd3Ll+1bH7bAXLPJjM9+Q/Gd14Dp0jV/pXnvY="}.FromBase64(),
ConstByteArray{"V2SfdhM12Acn1QD5+ja30vLaFTkFxGgo+c1hb4adiSw="}.FromBase64(),
ConstByteArray{"QQ8u6lD0h48OSGa8iJZlLKAFczncc5f6HeGbbtU6kfc="}.FromBase64(),
ConstByteArray{"SxFrk1AodiYJ8bkAzOyebpwv8Ky/Q96u5XXoB/xUVvI="}.FromBase64(),
ConstByteArray{"4v6oNOu1yt0Dah8s/oKTjBx5HBwfuW0A4oGNJeovcek="}.FromBase64(),
ConstByteArray{"ykpdRs1syysY0jEQwe8ThoidIdsd7L9fL+NkI5DgwZU="}.FromBase64(),
ConstByteArray{"DpXxMaDFrZxjZepVL45dGNApIEEUmhKRXCIUhN9zCCU="}.FromBase64(),
ConstByteArray{"hWRQjNXfslGPLWXVQ7AfKMK5ZZxA9RPHjEyHZv4bgFI="}.FromBase64(),
ConstByteArray{"5jQG+Ryl8wyNnm7yL8Jb82btzc8yFgs3Jy6UkA4ZUzo="}.FromBase64(),
ConstByteArray{"JaGR7769eNXFc1gCz44t04E82w4BCD1N/ApbNa0dpME="}.FromBase64(),
ConstByteArray{"x0RcJmtkrFg4nDfO8c1wagZu6X8OL6IMcvKpjiH+FHQ="}.FromBase64(),
ConstByteArray{"76Y90mZmMLE0qU8iElxP+hNnYLbAog+kSakrq/udu/c="}.FromBase64(),
ConstByteArray{"zUAoSVnDbey3RaDNN0Cn6rvJAvIukeNyuxueoHJvydo="}.FromBase64(),
ConstByteArray{"6+aTu0UPERkWe3QMqvwPsLeUGpe3W8pzndE6nrxDBwA="}.FromBase64(),
ConstByteArray{"1Lf6Gw+SplnJdvHnJhEQtoF7FxGj0vTQlw6t4xJ+NyE="}.FromBase64(),
ConstByteArray{"EIP5uUtJZaFas3ILt/dxXUssHX6SgtCK3ynUk33qVKI="}.FromBase64(),
ConstByteArray{"2Bu5oEd8xVabVY7FOplgbJsuXsV1TNYx5LIJkGQq/b4="}.FromBase64(),
ConstByteArray{"hLnOs5/N89bXmJoIThS5RL3g7yoxmeAIwIGOo87PSsk="}.FromBase64(),
ConstByteArray{"QhT9+L6gLWXs0b+NcATqOqDyMda67G1WPbYJMPZuSSU="}.FromBase64(),
ConstByteArray{"AnBBPxdafHujhi/JpYM8gA4lJOXu+ke4GWouEZeqkQY="}.FromBase64(),
ConstByteArray{"xUNohWLjuwz7br86E/DHUN0wN6nMG7Svt7mPXZZbIlc="}.FromBase64(),
ConstByteArray{"C5vkEWqQ5Ic96tD5tXZq45FUe/3dhPp5ITEZ2iz5wVI="}.FromBase64(),
ConstByteArray{"keQYOehz6QsDk7wU0rZuXgHMZcdE0ghSSvP0e+cun7U="}.FromBase64(),
ConstByteArray{"Nxn2cT7UNyZdlEJOXm6pKfq35LVLG5sYUxIKzY/noBc="}.FromBase64(),
ConstByteArray{"IYmEHHsIEjCXLWkk4w/jZqXujtuR2APXvpG9iUsARlQ="}.FromBase64(),
ConstByteArray{"mCsQyQ57fD1NPpCspAxM99IVG3J1iIN4A2/FcHV7yt0="}.FromBase64(),
ConstByteArray{"nTsC7oS/A7EinZLjqEOyeqNhOexsQmR685Dlq8Y+gZg="}.FromBase64(),
ConstByteArray{"WVaFVNXK0AmEPTQHB63wlE/my97QgnDNs1HRGrggRko="}.FromBase64(),
ConstByteArray{"vCjZlSsZD5Nyx4n+RpZxj+ouq6VnJsrnjpfA1sF+SCk="}.FromBase64(),
ConstByteArray{"K8f2wla4Svw9rhd0VCVJ05sbzqhhMVjFnYsrD05MrL4="}.FromBase64(),
ConstByteArray{"6WDra3Uxr8zqd+Otn9839tzNlxMeU8vCwfT7LIpNI3Q="}.FromBase64(),
ConstByteArray{"tPHpVGOIlJhOZKIAmEqTOhW+6MykfFIrgtwIY5dqrzM="}.FromBase64(),
ConstByteArray{"zTrtqKbCFJGfUoTIknwbP3YIwFr3cxAO1k7OW+H9Qu8="}.FromBase64(),
ConstByteArray{"fxuiVwlLhCsiG7ZApcGpb06g2RgeaMsu5L1STc2Uf3g="}.FromBase64(),
ConstByteArray{"Fd70lH+JRwkVZiiiSrxaTYpcVc22tEQ3QBEDt0GOUfI="}.FromBase64(),
ConstByteArray{"gnGkvbWCNHEEBKA1iueerPK1E4a+LwFzhvz0zYxUHdE="}.FromBase64(),
ConstByteArray{"ayuHmYJgKvs4gsVmCDHYu3FomJhRaYQCbeqMw82/rxo="}.FromBase64(),
ConstByteArray{"NlJZq+8xoZntN6Un6yyTTLVXxJHIu/3gz5hB7flfSNg="}.FromBase64(),
ConstByteArray{"ZeZA8z/P/cbqpkcTRLHSqq1SdPRvQY9OueBEfrA8qMQ="}.FromBase64(),
ConstByteArray{"hUr6xhNxbZGJ2DBX23YsNb86m3njwUD82udGRt+s/Ic="}.FromBase64(),
ConstByteArray{"7flWB5JLJsj87/z7slasgYQS+5R2tdPJcsBqmQo7x3s="}.FromBase64(),
ConstByteArray{"8qJdDchTAypvOZQaMQK3CDk2wazIn55Rys9xjVwTER0="}.FromBase64(),
ConstByteArray{"DZAzDrEIXxcLh/ogl6MdL0eMKWpuwzvHi1vJuBJO/7I="}.FromBase64(),
ConstByteArray{"sjuDQSM6kD/pPZAySq8F/IX5LsaPVFxyOtYuWSYnZQc="}.FromBase64(),
ConstByteArray{"lYjtIrDaOSAbly/KJqsadoc4APEc7gPonx0ZkvhbbAU="}.FromBase64(),
ConstByteArray{"9ecI2fpUKhZ6lg3iYkAYphIpbNnbKnLF/ytWkPfn0lI="}.FromBase64(),
ConstByteArray{"v8crJV5jqOfCGo4v3jUNJz+ApRHUq6APU7pDnMFXJ5Y="}.FromBase64(),
ConstByteArray{"P86149kM3a+ObmyNc9N9se1ZpKtVj2wZ0W8anpHHnZI="}.FromBase64(),
ConstByteArray{"TKg9RG+9K+ZyDElYwpm2NnRSEMBqjceIX2FM11rebL0="}.FromBase64(),
ConstByteArray{"H8SdON6sq5DYDMBesvsjDYKL79ut1wgT6B8KXOQ4tDw="}.FromBase64(),
ConstByteArray{"DJyROMNX0eCDnkI2z2oV9spnZE8Za+lQjZoP+TF86RM="}.FromBase64(),
ConstByteArray{"L333B6sifZ2LujnwZg/kCM6Pn6+j7Q2EvPxFxo7adiM="}.FromBase64(),
ConstByteArray{"1KP0V3DLAmC6rYJHZ8DQkIvqs0T0cchDTmpREGWSS1Q="}.FromBase64(),
ConstByteArray{"TbW58ueMaVTHQ18B5ABLybb4rqFRor10T38A530CLzI="}.FromBase64(),
ConstByteArray{"VrHeg6XuuW6KdsqTe3YbwACRpuTqvPxZ2iNJXNYLoos="}.FromBase64(),
ConstByteArray{"E2paEZ95kcB7aIV+a85te7tuIjU3TmBsNMV/p7nVhR4="}.FromBase64(),
ConstByteArray{"V7UZRk/NpAdJmBeqCHvyZpOwCWiK3N4LRY0PO7YlLTA="}.FromBase64(),
ConstByteArray{"RiTeGki9NQWyRiEGw6UnNsYhOKLgV26e77b4ABkhGgg="}.FromBase64(),
ConstByteArray{"m6IaFFeDKawuNq9LI/Ca4PNwrGrWn67vGpwbCvKb0+Y="}.FromBase64(),
ConstByteArray{"nYnqOTQ/IGNKIEVcxKm50AbuMRF7+KcRtf80n9AQzXQ="}.FromBase64(),
ConstByteArray{"tkCJwnSkqpVZ+1RACdk2YLEystuJi04L3bv15+rT9bY="}.FromBase64(),
ConstByteArray{"JMgY98XNR4nWm0BbygS9ZMjVQvkxsD4huxOsGKapEB8="}.FromBase64(),
ConstByteArray{"g1RAUmnVcoqDR6Yj7jwsRo389qZoukePgEmTrR2cNt8="}.FromBase64(),
ConstByteArray{"L3nwXmcqwJzu9kwpYnlPQu9R2k49w7jG7q8vahSa3bI="}.FromBase64(),
ConstByteArray{"U/YxXnsk6kQD2grcSv4JqIj9E8cliu+MjBjNdX16+lE="}.FromBase64()};

// Taken from transaction layout code
uint64_t GetIndex(ConstByteArray const &pub_key)
{
  uint32_t const log2_num_lanes = 8;

  auto address = Address(fetch::crypto::Identity{pub_key});

  ConstByteArray const resource = "fetch.token.state." + address.display();

  // compute the resource address
  fetch::storage::ResourceAddress const resource_address{resource};

  // update the shard mask
  return resource_address.lane(log2_num_lanes);
}

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
    uint64_t lane_index = GetIndex(pub_key);

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

      /*
      auto tx_layout_16 = fetch::chain::TransactionLayout{*tx, 4};
      auto tx_layout_256 = fetch::chain::TransactionLayout{*tx, 8};

      FETCH_UNUSED(tx_layout_16);
      FETCH_UNUSED(tx_layout_256);

      if(tx_layout_16.mask().PopCount() != 1)
      {
        FETCH_LOG_INFO(std::to_string(lane).c_str(), "thing thing thing", tx_layout_16.mask().PopCount());
      }
      */

      // serialise the transaction
      TransactionSerializer serializer{};
      serializer << *tx;

      std::lock_guard<std::mutex> lock(transactions_mutex);
      transactions.emplace_back(serializer.data());
      /* original_txs.emplace_back(*tx); */
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
