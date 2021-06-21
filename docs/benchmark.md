
## FastCFS vs. Ceph benchmark under 3 nodes cluster (three copies of data)

<table border=1 cellspacing=0 cellpadding=4 width=640 style="border:2px solid">
<thead>
<tr style="background-color:#D3D3D3">
<td width=128 rowspan=2 align=center><b>read/write type</b></td>
<td width=128 rowspan=2 align=right><b>fio jobs &nbsp;</b></td>
<td width=256 colspan=2 align=center><b>IOPS（4KB Block）</b></td>
<td width=128 rowspan=2 align=right><b>ratio &nbsp;</b></td>
</tr>
<tr style="background-color:#D3D3D3">
<td width=128 align=right><b>FastCFS &nbsp;</b></td>
<td width=128 align=right><b>Ceph &nbsp;</b></td>
</tr>
</thead>

<tr style="background-color:#F5F5F5">
<td width=128 rowspan=3 align=center><b>sequential write</b></td>
<td width=128 align=right>4 &nbsp;</td>
<td width=128 align=right>32,256 &nbsp;</td>
<td width=128 align=right>5,120 &nbsp;</td>
<td width=128 align=right>630% &nbsp;</td>
</tr>

<tr style="background-color:#F5F5F5">
<td align=right>8 &nbsp;</td>
<td align=right>55,296 &nbsp;</td>
<td align=right>8,371 &nbsp;</td>
<td align=right>661% &nbsp;</td>
</tr>

<tr style="background-color:#F5F5F5">
<td align=right>16 &nbsp;</td>
<td align=right>76,800 &nbsp;</td>
<td align=right>11,571 &nbsp;</td>
<td align=right>664% &nbsp;</td>
</tr>

<tr>
<td rowspan=3 align=center><b>rand write</b></td>
<td align=right>4 &nbsp;</td>
<td align=right>6,374 &nbsp;</td>
<td align=right>4,454 &nbsp;</td>
<td align=right>143% &nbsp;</td>
</tr>

<tr>
<td align=right>8 &nbsp;</td>
<td align=right>11,264 &nbsp;</td>
<td align=right>6,400 &nbsp;</td>
<td align=right>176% &nbsp;</td>
</tr>

<tr>
<td align=right>16 &nbsp;</td>
<td align=right>16,870 &nbsp;</td>
<td align=right>7,091 &nbsp;</td>
<td align=right>238% &nbsp;</td>
</tr>

<tr style="background-color:#F5F5F5">
<td rowspan=3 align=center><b>sequential read</b></td>
<td align=right>4 &nbsp;</td>
<td align=right>34,880 &nbsp;</td>
<td align=right>14,848 &nbsp;</td>
<td align=right>235% &nbsp;</td>
</tr>

<tr style="background-color:#F5F5F5">
<td align=right>8 &nbsp;</td>
<td align=right>62,751 &nbsp;</td>
<td align=right>24,883 &nbsp;</td>
<td align=right>252% &nbsp;</td>
</tr>

<tr style="background-color:#F5F5F5">
<td align=right>16 &nbsp;</td>
<td align=right>86,508 &nbsp;</td>
<td align=right>38,912 &nbsp;</td>
<td align=right>222% &nbsp;</td>
</tr>

<tr>
<td rowspan=3 align=center><b>rand read</b></td>
<td align=right>4 &nbsp;</td>
<td align=right>12,527 &nbsp;</td>
<td align=right>12,160 &nbsp;</td>
<td align=right>103% &nbsp;</td>
</tr>

<tr>
<td align=right>8 &nbsp;</td>
<td align=right>23,610 &nbsp;</td>
<td align=right>22,220 &nbsp;</td>
<td align=right>106% &nbsp;</td>
</tr>

<tr>
<td align=right>16 &nbsp;</td>
<td align=right>41,790 &nbsp;</td>
<td align=right>36,608 &nbsp;</td>
<td align=right>114% &nbsp;</td>
</tr>
</table>

more info see Chinese doc: [FastCFS vs. Ceph benchmark detail](benchmark-20210621.pdf)
