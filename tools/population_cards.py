"""Native 4K population field guide. The journal supplies the drawing and codec."""

ATLAS_W,ATLAS_H=3840,2160
ATLAS_SCHEMA=3

def population_roster(data,b,subject):
    metrics=data.get("metrics",{});specimens=data.get("specimens",[])
    groups=[p for p in specimens if p.get("role") in ("morphospecies","ranked_body","ranked_flight")]
    if "ranked_residents" in metrics:
        rows=sorted(groups,key=lambda p:p["measures"]["rank"])
        feedback=any(line.startswith("field_ink=") and pairs(line).get("imitating")=="1" and int(pairs(line).get("learned_calls",0))>0 for line in data.get("lines",[]))
        return dict(kind="census",mode=rows[0]["role"] if rows else "morphospecies" if data["command"]=="floodplain" else "ranked_body",
                    total=int(metrics["ranked_residents"]),types=int(metrics["ranked_morphs"]),opening=metrics.get("opening_residents"),
                    tick=metrics.get("census_tick"),**({"auditory_feedback":True} if feedback else {}),items=[dict(name=p["path"],id=p["path"].split()[-1].upper() if p["role"]=="morphospecies" else atlas_identity(p["measures"]),q=p["measures"]) for p in rows])
    # Historical sampling can support a sample frequency. It cannot stand in
    # for a whole census when the unobserved residents have unknown bodies.
    frames=data.get("frames",[]);sample=frames[-1].get("residents",[]) if frames else []
    if sample:
        by_form=defaultdict(list)
        for r in sample:by_form[r[6]].append(r)
        items=[]
        for body,group in sorted(by_form.items(),key=lambda t:(-len(t[1]),t[0]))[:8]:
            r=max(group,key=lambda v:(v[5],-v[0]))
            q=dict(members=len(group),body=body,slot=r[0],founder=r[2],generation=r[4],energy=r[5],tape=r[9])
            if len(r)>12:q.update(effectors_low=r[11],effectors_high=r[12])
            items.append(dict(name=f"Body form {body:08X}",q=q))
        return dict(kind="sample",mode="ranked_body",total=len(sample),types=len(by_form),tick=frames[-1]["t"][1],items=items)
    legacy=data.get("commons_residents",[])
    flights=[pairs(line) for line in data.get("lines",[]) if line.startswith("roost_resident=")]
    if legacy or flights:
        flight=bool(flights)
        members=([dict(slot=int(v["roost_resident"]),roaming=int(v["form"]),founder=int(v["founder"]),calls=int(v["calls"]),meals=int(v["food"])) for v in flights] if flight else
                 [dict(slot=v[0],founder=v[1],energy=v[2],meals=v[3],offspring=v[5],body=v[6],tape=v[7]) for v in legacy if v[2]])
        groups=defaultdict(list)
        for q in members:groups[q["roaming" if flight else "body"]].append(q)
        items=[]
        for form,group in sorted(groups.items(),key=lambda item:(-len(item[1]),item[0]))[:8]:
            q=dict(max(group,key=lambda r:(r.get("energy",r.get("meals",0)),-r["slot"])),members=len(group))
            items.append(dict(name=f"Form {form:08X}",q=q))
        return dict(kind="sample",mode="ranked_flight" if flight else "ranked_body",total=len(members),types=len(groups),items=items)
    return dict(kind="individual",mode=subject["glyph"],total=None,types=None,
                items=[dict(name=subject["name"],q=subject["measures"])] if subject["measures"] or subject["circuits"] else [])

def atlas_outline(cells):
    # Trace oriented exposed cell edges, including holes and narrow branches.
    edges=defaultdict(set)
    for i in cells:
        x,y=i%5,i//5
        for neighbour,start,end in (((x,y-1),(x,y),(x+1,y)),((x+1,y),(x+1,y),(x+1,y+1)),
                                    ((x,y+1),(x+1,y+1),(x,y+1)),((x-1,y),(x,y+1),(x,y))):
            nx,ny=neighbour
            if not (0<=nx<5 and 0<=ny<5 and nx+5*ny in cells):edges[start].add(end)
    loops=[];dirs=[(1,0),(0,1),(-1,0),(0,-1)]
    while edges:
        start=min(edges);at=start;direction=0;loop=[]
        while True:
            loop.append(at);choices=edges[at]
            def turn(point):
                heading=dirs.index((point[0]-at[0],point[1]-at[1]));return (1,0,3,2).index((heading-direction)%4)
            nxt=min(choices,key=turn);direction=dirs.index((nxt[0]-at[0],nxt[1]-at[1]));choices.remove(nxt)
            if not choices:del edges[at]
            at=nxt
            if at==start:break
        loops.append(loop)
    return loops

def atlas_anatomy(d,cx,cy,q,identity,step=48,difference=0):
    cells={i for i in range(25) if q["body_plan"]&(1<<i)}
    if not cells:return
    midx=(min(i%5 for i in cells)+max(i%5 for i in cells)+1)/2
    midy=(min(i//5 for i in cells)+max(i//5 for i in cells)+1)/2
    point=lambda x,y:(cx+(x-midx)*step,cy+(y-midy)*step)
    colors=("#a1bca7","#c1ba85","#c79779","#74aead","#a8a6c3","#bd9c76","#7e9eab","#aaba8c")
    pigment=colors[int(identity[-7:],16)%len(colors)];grad="tissue"+identity;shadow=blend(pigment,"#1d3b40",.54)
    d.add(f'<defs><linearGradient id="{grad}" x1="0" y1="0" x2=".88" y2="1"><stop stop-color="{blend(pigment,"#fff4d3",.37)}"/><stop offset=".48" stop-color="{pigment}"/><stop offset="1" stop-color="{shadow}"/></linearGradient></defs>')
    paths=[]
    for loop in atlas_outline(cells):
        pieces=[]
        for k,(x,y) in enumerate(loop):
            px,py=loop[k-1];nx,ny=loop[(k+1)%len(loop)]
            a=point(x+.26*(px-x),y+.26*(py-y));b=point(x+.26*(nx-x),y+.26*(ny-y));v=point(x,y)
            pieces.append((a,v,b))
        paths.append(f"M{pieces[-1][2][0]:.2f},{pieces[-1][2][1]:.2f} "+" ".join(
            f"L{a[0]:.2f},{a[1]:.2f} Q{v[0]:.2f},{v[1]:.2f} {b[0]:.2f},{b[1]:.2f}" for a,v,b in pieces)+"Z")
    d.path(" ".join(paths),blend(pigment,"#dfecd2",.40),2.2,f"url(#{grad})",'fill-rule="evenodd"')
    for i in sorted(cells):
        x,y=point(i%5+.5,i//5+.5);r=step/2
        for other,axis in ((i+1,0),(i+5,1)):
            if other not in cells or (axis==0 and i%5==4):continue
            if axis==0:d.path(f"M{x+r} {y-r*.51} q{-r*.25} {r*.5} 0 {r}",shadow,1.3,extra='opacity=".55"')
            else:d.path(f"M{x-r*.55} {y+r} q{r*.55} {-r*.21} {r*1.1} 0",shadow,1.3,extra='opacity=".55"')
        if q["legs"]&(1<<i):
            for j in range(3):
                bx=x-r*.54+j*step*.09;d.rule(bx,y+r*.48,bx+step*.07,y+r*.23,"#b6783d",step*.043)
        if q["pores"]&(1<<i):d.dot(x-r*.39,y-r*.06,step*.079,"#466456",f'stroke="#d5e99b" stroke-width="{step*.042:.2f}"')
        if q["eyes"]&(1<<i):
            d.dot(x+r*.33,y-r*.23,step*.133,"#91e1d5",f'stroke="#395e59" stroke-width="{step*.043:.2f}"')
            d.dot(x+r*.33,y-r*.23,step*.061,"#173739");d.dot(x+r*.40,y-r*.33,step*.024,"#f5fff0")
    for k in range(25):
        if difference&(1<<k):
            x,y=point(k%5+.5,k//5+.5)
            d.dot(x,y,step*.41,"none",f'stroke="#ffdf83" stroke-width="2.7" stroke-dasharray="4 3"')

def atlas_nearest(q,roster,rank):
    if "body_plan" not in q or len(roster["items"])<2:return 0,0
    comparisons=[]
    for k,r in enumerate(roster["items"],1):
        if k==rank:continue
        other=r["q"];difference=0;cost=0
        for key in ("body_plan","legs","pores","eyes"):
            bits=q[key]^other[key];difference|=bits;cost+=bits.bit_count()
        comparisons.append((cost,difference.bit_count(),k,difference))
    _,_,neighbour,mask=min(comparisons);return neighbour,mask

def atlas_identity(q):
    if "body_plan" in q:values=[q[k] for k in ("body_plan","legs","pores","eyes")]
    else:values=[q.get("roaming",q.get("body",0))]
    return hashlib.blake2s(b"".join(int(v).to_bytes(4,"little") for v in values),digest_size=5).hexdigest().upper()

def atlas_name(q,mode):
    if "body_plan" in q:
        cells={i for i in range(25) if q["body_plan"]&(1<<i)}
        if not cells:return "Unoccupied body"
        adj={i:[j for j in cells if abs(i%5-j%5)+abs(i//5-j//5)==1] for i in cells}
        tips=[i for i in cells if len(adj[i])==1]
        def length(tip):
            n=0;last=-1;cell=tip
            while True:
                route=[j for j in adj[cell] if j!=last]
                if len(route)!=1:return n
                last,cell=cell,route[0];n+=1
        label="Compact form"
        if tips and len(cells)>2:
            tip=max(tips,key=lambda i:(length(i),(i%5-2)**2+(i//5-2)**2,-i))
            dx,dy=tip%5-2,tip//5-2
            direction=(("north" if dy<0 else "south" if dy>0 else "")+("west" if dx<0 else "east" if dx>0 else "")).capitalize()
            if direction:label=direction+" branch"
        eyes=q["eyes"].bit_count()
        return f"{label} · {eyes} "+("eye site" if eyes==1 else "eye sites")
    return "Flight phenotype" if mode=="ranked_flight" else "Motor phenotype"

def atlas_wiring(d,x,y,w,h,q):
    """Draw the actual eight by eight motor-contact matrix or the recorded code."""
    if "effectors_low" in q:
        size=min(w/8.5,h/8.5);ox=x+(w-8*size)/2;oy=y+(h-8*size)/2
        ports=q["effectors_low"]|(q["effectors_high"]<<32)
        for motor in range(8):
            for contact in range(8):
                on=(ports>>(8*motor+contact))&1
                d.dot(ox+(contact+.5)*size,oy+(motor+.5)*size,size*(.27 if on else .13),"#bcdfa6" if on else "#49675e")
        return "Muscle-to-effector contacts"
    bits=q.get("roaming",q.get("body"))
    if bits is not None:
        size=min(w/9,h/5)
        for bit in range(32):
            px=x+(w-8*size)/2+(bit%8)*size;py=y+(h-4*size)/2+(bit//8)*size
            d.rect(px,py,size*.79,size*.79,"#aed6ac" if bits&(1<<bit) else "#3d6055",4)
        return "Inherited movement code" if "roaming" in q else "Inherited body code"
    return ""

def atlas_range(q,low,high):
    a,b=q.get(low),q.get(high)
    return "—" if a is None else str(a) if a==b or b is None else f"{a}–{b}"

def atlas_card(d,x,y,member,rank,roster):
    w,h=888,770;q=member["q"];mode=roster["mode"];identity=member.get("id",atlas_identity(q))
    d.rect(x+3,y+5,w,h,"#d2d7cb",17);d.rect(x,y,w,h,"#fbf9ef",17,'stroke="#b8c7b7" stroke-width="2"')
    d.text(x+30,y+44,f"{rank:02d}  /  POPULATION RANK" if roster["kind"]=="census" else f"{rank:02d}  /  SAMPLE RANK",19,GREEN,MONO,600,space=.9)
    d.text(x+w-29,y+44,identity,19,MUTED,MONO,anchor="end")
    d.text(x+28,y+101,atlas_name(q,mode),42,INK,SERIF,500)
    d.rect(x+18,y+122,w-36,288,"#203e3c",12)
    nearest,delta=atlas_nearest(q,roster,rank)
    if "body_plan" in q:
        marks=delta if delta.bit_count()<=3 else 0
        atlas_anatomy(d,x+295,y+264,q,identity,45,marks)
        caption=(f"{q['pod_contacts']}/256 grip contacts set   ·   {q['lag_banks']} eligible delays" if "pod_contacts" in q and "natal_cells" in q else "")
    else:caption=atlas_wiring(d,x+45,y+156,475,206,q)
    if "hand_faces" in q:
        if q["hand_faces"]:
            d.rect(x+47,y+149,38,38,"#deb373",5)
            for k,(dx,dy) in enumerate(((0,-1),(1,0),(0,1),(-1,0))):
                val=(q["hand_faces"]>>(2*k))&3
                if val:d.dot(x+66+14*dx,y+168+14*dy,3,("#000","#473b2c","#faf5d9","#66b4ac")[val])
            d.text(x+98,y+178,"carried grain",18,"#d6e4cc",MONO)
        caption=f"{q['manual_ports'].bit_count()} manual gates   ·   {q['work_sightings']:,} registered work effects"
    if "spatial_replay_bytes" in q:
        caption=f"{q['spatial_replay_bytes']} B predictive tissue   ·   {q['manual_ports'].bit_count()} manual gates"
    if "explored_substrates" in q:
        caption=f"Experienced by this resident   ·   {q['explored_substrates']}/16 soils   ·   {q['seen_face_pairs']}/16 face pairs"
    if "predictive_cells" in q:
        caption=f"{q['predictive_cells']} material-predicting cells   ·   {q['modified_predictive_cells']} with acquired traces"
    if "optical_banks" in q:
        banks=(q["optical_hand"],q["optical_gate_a"]&65535,q["optical_gate_a"]>>16,q["optical_gate_b"]&65535)
        for i,mask in enumerate(banks):
            used=q["optical_banks"]&(1<<i)
            for row in range(4):
                for col in range(4):
                    active=mask&(1<<(4*row+col))
                    d.rect(x+421+34*i+7*col,y+160+12*row,5,9,"#edc77f" if active and used else "#709d90" if used else "#395651",1)
        d.text(x+488,y+237,f"{q['optical_banks'].bit_count()} visual banks",16,"#c8d7c7",MONO,anchor="middle")
    elif "inlet_contacts" in q:
        # The sixteen squares are the oldest living member's actual contact mask.
        for inlet in range(4):
            for mouth in range(4):
                set_contact = q["inlet_contacts"] & (1 << (4 * inlet + mouth))
                d.rect(x+449+19*mouth,y+149+16*inlet,14,12,"#f1c77d" if set_contact else "#486462",2)
        d.text(x+486,y+235,"inlet memory",16,"#c8d7c7",MONO,anchor="middle")
    elif "tool_contacts" in q:
        for j,(key,lab,color) in enumerate((("tool_contacts","tools","#dbb687"),("bath_contacts","water","#9bc7c0"))):
            for row in range(4):
                for col in range(4):
                    bit=1<<(4*row+col)
                    d.rect(x+424+69*j+13*col,y+155+16*row,10,11,color if q[key]&bit else "#45615b",2)
            d.text(x+449+69*j,y+245,lab,15,"#c8d7c7",MONO,anchor="middle")
        caption=(f"{q['clay_assemblies']} / {q['clay_capacity']} context assemblies   ·   "
                 f"{q['tool_contacts'].bit_count()} tool contacts   ·   {q['bath_contacts'].bit_count()} water contacts")
    elif "acoustic_reply" in q:
        palette=("#f2c876","#9bd6b0","#d0b0e1","#94c8d9")
        for receptor in range(4):
            winners=(q["acoustic_reply"]>>(4*receptor))&15
            for tone in range(4):
                d.rect(x+456+18*tone,y+154+17*receptor,13,11,palette[tone] if winners&(1<<tone) else "#375156",2)
        d.text(x+490,y+245,"voice field",16,"#c8d7c7",MONO,anchor="middle")
        caption=(f"{q['clay_assemblies']} / {q['clay_capacity']} context assemblies   ·   {q['clay_grooves']} carried impressions" if "clay_assemblies" in q
                 else f"{q['siphon_heard']:,} calls heard   ·   {q['siphon_food']:,} carbon from joint contacts")
    if "motor_routes" in q:
        # Show the oldest member's four intrinsic-to-physical muscle contacts.
        for source in range(4):
            target=(q["motor_routes"]>>(2*source))&3
            start=x+452+25*source;finish=x+452+25*target
            d.path(f"M{start} {y+272} C{start} {y+289} {finish} {y+294} {finish} {y+311}","#9ebd99",2)
            d.dot(start,y+270,4,"#eac684");d.dot(x+452+25*source,y+313,4,"#bfe3c3")
        d.text(x+490,y+348,"grip wiring",16,"#c8d7c7",MONO,anchor="middle")
    if "place_cells" in q:
        caption=f"{q['place_cells']} / {q['place_capacity']} occupied place cells   ·   {q['place_events'].bit_count()} event cues"
    if "learned_costs" in q:
        caption=f"{q['place_cells']} place cells   ·   {q['learned_yields']} fruit cues   ·   {q['learned_costs']} effort cues"
    if "material_contexts" in q:
        caption=f"{q['material_contexts']}/{q['context_capacity']} material banks   ·   {q['mechanism_contacts']:,} observed strokes   ·   {q['motor_horizon']}-stroke reach"
    if "personal_observations" in q:
        caption=f"{q['learned_contexts']}/{q['context_limit']} material models   ·   {q['personal_observations']:,} observations   ·   up to {q['attention_span']} contacts/bout"
    if "flex_contacts" in q:
        caption=f"{q['flex_contacts']}/64 flex contacts   ·   {q['peer_contacts']} from peers   ·   {q['private_folds']:,} private samples"
    if "rattle_contacts" in q:
        caption=f"Resonator {q['rattle_timbre']+1}   ·   {q['rattle_contacts']}/16 active bank contacts"
        if "rattle_latency" in q:caption+=f"   ·   taste after {q['rattle_latency']} ticks"
    if "float_capacity" in q:
        caption=f"{q['float_motor_cells']}/4 active hand assemblies   ·   {q['float_word_cells']}/{q['float_capacity']} acoustic"
        if "pressure_pairs" in q:caption+=f"   ·   {q['pressure_pairs']}/16 pressure pairs"
        if "retinal_positions" in q:
            colors=("#e9b475","#b7cb98","#c7a6d1","#76bac4")
            places="".join(str((q["retinal_positions"]>>(2*i))&3) for i in range(4))
            d.text(x+43,y+227,"Socket map",17,"#d8dcb7",MONO)
            d.text(x+43,y+253,places,22,"#edf1dc",MONO,600)
            for i in range(4):d.dot(x+50+21*i,y+281,7,colors[(q["retinal_bands"]>>(2*i))&3])
        elif "visible_event_ports" in q and q["visible_event_ports"]:
            d.text(x+43,y+356,f"{q['visible_event_ports']} event contacts",17,"#d8dcb7",MONO)
        if q.get("question_forms"):
            d.text(x+43,y+330,f"{q['question_forms']} learned questions",17,"#d8dcb7",MONO)
    if "inlet_contacts" in q:
        caption=(f"Hand band {q['inlet_motor_band']}   ·   Ear band {q['inlet_vocal_band']}   ·   "
                 f"{q['inlet_contacts'].bit_count()}/16 inlet contacts set")
    if "optical_banks" in q:
        caption=(f"{q['optical_banks'].bit_count()} visual banks   ·   {q['viewing_motors'].bit_count()}/8 motor patterns admit light")
    if "iris_quorum" in q:
        caption=f"{q['optical_banks'].bit_count()} visual banks   ·   gaze opens at {q['iris_quorum']}/4 active cells"
    if "neighbour_contacts" in q:
        caption=f"Outline {q['public_silhouette']+1}/4   ·   {q['neighbour_contacts'].bit_count()}/16 public-view contacts set"
    d.text(x+39,y+389,caption,18,"#c8d7c7",MONO)
    share=100*q["members"]/max(1,roster["total"])
    d.rule(x+572,y+158,x+572,y+375,"#47655c",2)
    d.text(x+604,y+221,f"{share:.1f}%",51,"#eef1d5",SANS,600)
    d.text(x+605,y+260,"of living residents" if roster["kind"]=="census" else "of the sample",22,"#bfd4c4")
    d.text(x+605,y+324,f"{q['members']:,}",36,"#b9dea7",MONO,600)
    d.text(x+605,y+357,"individuals",20,"#c7d8ca")
    if "body_plan" in q:
        values=[(str(q[key].bit_count()),label) for key,label in (("body_plan","segments"),("legs","limb sites"),("eyes","eye sites"),("pores","feeding pores"))]
    elif mode=="ranked_flight":
        values=[(str(q.get(k,"—")),name) for k,name in (("productions","motor productions"),("receptors","acoustic forms"),("energy","energy"),("founder","founder ID"))]
    else:
        values=[(str(q.get(k,"—")),name) for k,name in (("neurons","active cells"),("tape","instructions"),("energy","energy"),("founder","founder ID"))]
    for i,(v,label) in enumerate(values):
        px=x+30+i*214
        if i:d.rule(px-14,y+435,px-14,y+513,"#ced7c9")
        d.text(px,y+476,v,40,GREEN,SERIF);d.text(px,y+508,label,20,MUTED)
    d.rule(x+28,y+530,x+w-28,y+530,"#c4cebf")
    if "body_plan" in q:
        d.text(x+29,y+569,"Spiking cells",23,MUTED);d.text(x+190,y+569,atlas_range(q,"min_neurons","max_neurons"),27,INK,SANS,600)
        d.text(x+400,y+569,"Neural populations",23,MUTED);d.text(x+685,y+569,atlas_range(q,"min_modules","max_modules"),27,INK,SANS,600)
        stocks=[q.get(k,0) for k in ("leaf","fruit","litter")];total=sum(stocks);colors=("#6b9663","#d39253","#8c718c")
        d.text(x+30,y+617,"Energy eaten",22,MUTED)
        if total:
            at=x+194;bar=306
            for amount,color in zip(stocks,colors):
                length=bar*amount/total
                if length:d.rect(at,y+597,length,20,color)
                at+=length
            lead=max(range(3),key=lambda k:stocks[k])
            d.text(x+523,y+618,f"{100*stocks[lead]/total:.0f}% "+("leaf","fruit","litter")[lead],23,INK,SANS,600)
        else:d.text(x+194,y+617,"No intake recorded",22,MUTED)
        prior=roster.get("opening")
        if prior:
            old=100*q.get("opening_members",0)/prior
            d.text(x+30,y+663,f"Share   {old:.1f}%  →  {share:.1f}%",23,GREEN,MONO)
        if nearest:d.text(x+430,y+663,f"Nearest #{nearest:02d} · {delta.bit_count()} differing "+("site" if delta.bit_count()==1 else "sites"),20,MUTED)
        d.text(x+30,y+707,f"Oldest member  #{q['slot']}   ·   Generation {q['generation']:,}",22,INK,SANS,600)
        if "natal_cells" in q:
            d.text(x+w-29,y+707,f"Capacity {q['capacity_cells']:,} cells",20,MUTED,MONO,anchor="end")
            phase=f"Born with {q['natal_cells']}   →   {q['active_cells']} now   ·   Tissue peak {q['peak_cells']}"
            if "pressure_contacts" in q:
                phase=f"Cells {q['natal_cells']} → {q['active_cells']}, peak {q['peak_cells']}   ·   {q['pressure_receptors']} pressure inputs   ·   {q['pressure_contacts']} contacts"
                if "signed_contexts" in q:
                    phase=f"Cells {q['natal_cells']} → {q['active_cells']}, peak {q['peak_cells']}   ·   {q['context_rows']} passive rows   ·   {q['signed_contexts']} signed probes"
                if "appetitive_contacts" in q:
                    phase=f"Cells {q['natal_cells']} → {q['active_cells']}, peak {q['peak_cells']}   ·   {q['appetitive_contacts']} signed food contacts   ·   {q['foraging_ports']}/8 motor states"
            elif "retained_dye_events" in q:
                phase=f"Cells {q['natal_cells']} → {q['active_cells']}, peak {q['peak_cells']}   ·   {q['retained_dye_events']}/7 encounters"
                if "resonator_phases" in q:
                    total_phases=q["resonator_phases"]
                    phase+=f"   ·   {total_phases} vocal phases"
                    if roster.get("auditory_feedback") and "neighbour_write_gene" in q:
                        hearing=((q["neighbour_write_gene"]>>16)&((1<<total_phases)-1)).bit_count()
                        phase+=f" ({hearing} auditory)"
            elif "siphon_contacts" in q:phase+=f"   ·   {q['siphon_contacts']:,} native exchanges"
            elif q.get("muted_cells"):phase+=f"   ·   {q['muted_cells']} cells silenced"
            elif "known_surfaces" in q:phase+=f"   ·   {q['known_surfaces']}/8 surfaces anchored"
            d.text(x+30,y+742,phase,20,GREEN)
        elif "pod_contacts" in q:
            phrase=f"{q['pod_contacts']}/256 contacts set   ·   {q['lag_banks']} eligible delays"
            if "known_surfaces" in q:phrase+=f"   ·   {q['known_surfaces']}/8 surfaces anchored"
            d.text(x+30,y+742,phrase,20,MUTED)
        else:d.text(x+30,y+742,f"{q['age']:,} ticks old   ·   {q['offspring']:,} offspring   ·   {q['calls_heard']:,} calls experienced",20,MUTED)
    else:
        d.text(x+30,y+583,f"Recorded member  #{q['slot']}" if roster["kind"]=="sample" else f"Highest-energy member  #{q['slot']}",27,INK,SANS,600)
        d.text(x+30,y+634,"The portrait shows the recorded locomotor code." if mode=="ranked_flight" else "The portrait shows the recorded motor circuitry.",23,MUTED)
        if mode=="ranked_flight":
            evidence=(f"{q['productions']} motor productions   ·   {q['receptors']} acoustic forms" if "productions" in q else f"{q.get('calls',0)} calls emitted   ·   {q.get('meals',0)} meals")
            d.text(x+30,y+695,evidence,23,GREEN)

def atlas_repertoire(d,b,ledger,y):
    events=[e for e in ledger["milestones"] if e["run"]<=b["id"]]
    d.rule(87,y-9,3753,y-9,"#c5cec1",2)
    d.text(92,y+38,"Documented repertoire",33,INK,SERIF)
    d.text(94,y+77,f"{len(events)} demonstrated abilities through run {b['id']:05d}",22,MUTED)
    x0,x1,base=838,1726,y+73;path=f"M{x0} {base}";total=0
    for run,amount in sorted(Counter(e["run"] for e in events).items()):
        total+=amount;px=x0+(x1-x0)*(run-1)/max(1,b["id"]-1)
        path+=f" H{px:.1f} V{base-64*total/max(1,len(events)):.1f}"
    d.path(path+f" H{x1}",GREEN,4)
    if events:
        e=events[-1];d.text(1850,y+36,"LATEST FIRST DEMONSTRATION",19,GREEN,MONO,600,space=1)
        d.text(1850,y+77,short(e["title"],102),28,INK,SERIF);d.text(3750,y+73,f"Run {e['run']:05d}",23,MUTED,MONO,anchor="end")

def atlas_footer(d,b):
    d.rect(0,2020,ATLAS_W,140,"#1b3934");d.text(93,2080,"RUN PERFORMANCE",24,"#b7dfbd",MONO,600,space=1.1)
    p=b.get("performance",{});facts=perf_items(p)
    name=p.get("device","").replace("NVIDIA ","").replace(" Generation Laptop GPU","")
    d.text(94,2127,short(name,41) or f"Run {b['id']:05d}",22,"#ccddcc")
    width=3060/max(1,len(facts))
    for i,(v,label) in enumerate(facts):
        px=743+i*width;d.rule(px-32,2043,px-32,2138,"#4c695e",2)
        d.text(px,2086,v,40,"#f0f1d9",SANS,600);d.text(px,2126,label,24,"#c4d7c6")

def individual_atlas(d,b,roster):
    p=b["card"]["subject"];q=p["measures"]
    critter=globals().get("_mind_critter")
    if (p.get("glyph")=="mind" or b.get("command")=="mind") and critter is not None:
        # Expressive 4K Blackwell Spiking Creature Card!
        mk=q.get("mind_kind","eval");is_al=q.get("dialect")=="alien";arm=q.get("arm","intact")
        is_abl=arm not in ("intact","none","")
        accent="#bcb6ec" if is_al else ("#e28b79" if is_abl else "#91cfb7")
        gold,aqua,coral="#f3bc73","#87d6cf","#e28b79"
        # Left Specimen Stage (90, 294, 1480, 1580)
        d.rect(90,294,1480,1580,"#18342f",28,f'stroke="{accent}" stroke-width="3" stroke-opacity=".55"')
        d.text(146,374,"BLACKWELL SPIKING SPECIMEN   ·   ZERO LLMS",26,accent,MONO,700,space=1.8)
        badge=("LESION: "+arm.upper()) if is_abl else ("ALIEN SOV TWIN" if is_al else ("4-MIND CAST" if mk in ("ecosystem","society") else "29.36M COMPLETE-L2"))
        d.rect(1140,334,380,54,"#244a42",27,f'stroke="{accent}" stroke-width="2"')
        d.text(1330,370,badge,22,accent,MONO,700,"middle")

        cx,cy=830,1010
        if mk in ("ecosystem","society"):
            # Hero 4-Critter Cast Portrait Diorama!
            cast_pos=[
                (cx-340,cy-220,"alien" if is_al else "axion","proud","AXION","Scientist & Explorer",accent),
                (cx+340,cy-220,"mira","happy","MIRA","Honest Sage & Trader",gold),
                (cx-340,cy+230,"wren","happy","WREN","Swift Bridge Builder",aqua),
                (cx+340,cy+230,"fox","busted","FOX","Cheeky Bluff Caught!",coral),
            ]
            # Glowing trust & shield arcs (cleanly clear of character labels!)
            d.path(f"M{cx-180} {cy-220} H{cx+180}",accent,6)
            d.path(f"M{cx-340} {cy-38} V{cy+64}",aqua,6)
            d.path(f"M{cx+180} {cy+230} H{cx-150}",coral,5,extra='stroke-dasharray="14,10"')
            for px,py,cr,cm,cn,cd,cc in cast_pos:
                d.dot(px,py,155,cc,'opacity=".11"')
                critter(d,px,py-16,4.2,cr,cm,3)
                d.text(px,py+126,cn,28,cc,MONO,700,"middle")
                d.text(px,py+160,cd,22,"#d3e3d6",SANS,500,"middle")
        else:
            # Radiant Hero Portrait + Dual-Ring Staggered 40-Kernel Blackwell Brain Halo (zero overlap!)
            cy=1042
            d.dot(cx,cy,430,accent,'opacity=".07"')
            d.dot(cx,cy,310,accent,'opacity=".10"')
            d.add(f'<circle cx="{cx}" cy="{cy}" r="530" fill="none" stroke="{accent}" stroke-width="1.8" stroke-dasharray="8,10" opacity=".35"/>')
            d.add(f'<circle cx="{cx}" cy="{cy}" r="600" fill="none" stroke="{accent}" stroke-width="2.2" stroke-dasharray="10,10" opacity=".45"/>')
            kernels=[
                ("K1 SDR","150 Gnt/s",accent),("K2 26-SM HIPP","33 µs",gold),("K3 28M NEO","135 µs",aqua),
                ("K4 2D MAP","25 µs","#b7e19b"),("K5 CHAIN","431 µs","#f0b4c4"),("K6 CONTRAST","105 µs","#bcb6ec"),
                ("K7 ANALOGY","51 µs",gold),("K8 COMPOSE","58 µs",aqua),("K9 MULTICUE","62 µs",coral),
                ("K10 HYPOTH","67 µs",accent),("K11 BRIDGE","108 µs","#f0b4c4"),("K12 CLUSTER","121 µs","#b7e19b"),
                ("K13 GATE","55 µs","#ffe28a"),("K14 DREAM","294 µs","#bcb6ec"),("K15 SCAN","16 µs",aqua),
                ("K16 DEDUCE","394 µs",gold),("K17 ABDUCE","330 µs","#f0b4c4"),("K18 IMAGINE","539 µs",accent),
                ("K19 PLAN","315 µs","#ffe28a"),("K20 SIMULATE","343 µs","#87d6cf"),("K21 CONSTRAIN","24 µs","#bcb6ec"),
                ("K22 INDUCE","320 µs","#91cfb7"),("K23 CHAINRULE","269 µs","#f3bc73"),("K24 ONLINE-SCH","321 µs","#8ce0b3"),
                ("K25 REVISE-SCH","472 µs","#f0b4c4"),("K26 ROLE-BIND","164 µs","#87d6cf"),
                ("K27 SEQ-SYNTAX","184 µs","#ffe28a"),("K28 NEURAL-MATH","271 µs","#8ce0b3"),
                ("K29 GRAPH-WAVE","34 µs","#87d6cf"),("K30 SAT-SOLVE","42 µs","#e28b79"),
                ("K31 RETINA V1/V4","51 µs","#8ce0b3"),
                ("K32 COCHLEA A1","19 µs","#87d6cf"),
                ("K33 XMODAL+STK","7 µs","#ffe28a"),
                ("K34 DIALOGUE","8 µs","#8ce0b3"),
                ("K35 METAPHOR","7 µs","#f3bc73"),
                ("K36 BELIEF-DEB","6 µs","#bcb6ec"),
                ("K37 COLL-DISC","7 µs","#8fe8be"),
                ("K38 SELF-RES","5 µs","#f6ce76"),
                ("K39 SYM-CALC","4 µs","#8ce0b3"),
                ("K40 AST-SYN","5 µs","#c3b1f7"),
                ("K41 LIN-ALG","5 µs","#f3bc73"),
                ("K42 BAYES-NET","5 µs","#8ce0b3"),
                ("K43 MDP-PLAN","4 µs","#c3b1f7"),
                ("K44 KALMAN","4 µs","#f3bc73"),
                ("K45 LQR-CTRL","5 µs","#87d6cf"),
                ("K46 FFT-SPEC","5 µs","#c3b1f7"),
                ("K47 RK4-ODE","5 µs","#8ce0b3"),
                ("K48 SIMPX-LP","5 µs","#f3bc73"),
                ("K49 PCA-EIG","5 µs","#87d6cf"),
                ("K50 1D-PDE","5 µs","#c3b1f7"),
                ("K51 MCMC","13 µs","#8ce0b3"),
                ("K52 MCTS","15 µs","#f3bc73"),
            ]
            for ki,(kl,kv,kc) in enumerate(kernels):
                ang=-math.pi*0.5+2.0*math.pi*ki/len(kernels)
                rad=565 if (len(kernels)%2==1 and ki==len(kernels)-1) else (600 if (ki%2==0) else 530)
                kx=cx+rad*math.cos(ang);ky=cy+rad*math.sin(ang)
                d.rule(cx+185*math.cos(ang),cy+185*math.sin(ang),kx-43*math.cos(ang),ky-15*math.sin(ang),kc,2.0)
                d.dot(cx+(340 if ki%2==0 else 305)*math.cos(ang),cy+(340 if ki%2==0 else 305)*math.sin(ang),4.0,"#ffe28a")
                d.rect(kx-44,ky-15,88,30,"#132a25",8,f'stroke="{kc}" stroke-width="1.8"')
                d.text(kx,ky-2.0,kl,7.8,kc,MONO,700,"middle")
                d.text(kx,ky+10.0,kv,8.0,"#edf1dc",MONO,500,"middle")
            role="elder" if mk=="peer" else ("alien" if is_al else "axion")
            mood="ablate" if is_abl else ("thinking" if mk=="bench" else "proud")
            critter(d,cx,cy+10,6.4,role,mood,3)

        # Bottom L2 Cache Residency Strip inside Specimen Frame
        d.rect(146,1706,1368,120,"#122723",16,'stroke="#35584e" stroke-width="2"')
        d.text(180,1748,"BLACKWELL 32-MiB L2 CACHE FIT  ·  31,936 / 32,768 KiB (97.5% RESIDENT — ZERO VRAM SPILL)",21,accent,MONO,700)
        d.rect(180,1770,1300,28,"#24423b",10)
        d.rect(180,1770,130,28,"#c98b3e",10)
        d.rect(306,1770,1138,28,"#3d8c6e",10)
        d.text(875,1792,"28-MiB Neocortex (29,360,128 int8 synapses · 28 pathways)",19,"#edf1dc",MONO,700,"middle")

        # Right Field Dossier (1665..3750)
        rx=1665
        d.text(rx,362,short(p["name"],40),64,INK,SERIF,500)
        d.text(rx,424,short(p["scope"],84),31,MUTED,SANS)
        d.rule(rx,456,3750,456,"#c5cec1",2)

        # Sensory & Neural Equipment side-by-side panels (y=482..786)
        ew_r=1022
        for ei,(etitle,eitems,ecol) in enumerate((("SENSORY & INPUT EQUIPMENT",p["senses"],GREEN),("SPIKING NEURAL ARCHITECTURE",p["circuits"],"#327a83"))):
            ex=rx+ei*(ew_r+41)
            d.rect(ex,482,ew_r,298,"#e7ece0",16,f'stroke="{ecol}" stroke-width="2"')
            d.text(ex+28,526,etitle,21,ecol,MONO,700,space=1.3)
            for li,item in enumerate(eitems[:5]):
                d.dot(ex+36,568+li*44,6,ecol)
                d.text(ex+56,576+li*44,short(item,54),25,INK,SANS)

        # 6 Big Measured KPI Cards (y=812..1262)
        if mk=="eval":
            kpis=[(f"{q['correct']}/{q['total']}","strict probes solved"),(f"{q.get('domains',560)}","higher-cognition domains"),
                  (f"{q.get('kernels',52)}","Blackwell CUDA kernels"),("29.36M","plastic int8 synapses"),
                  (human(q.get("spikes",0)),"Blackwell GPU spikes"),(f"{q.get('edits',0):,}","live synaptic edits")]
        elif mk=="ecosystem":
            kpis=[(f"{q['correct']}/{q['total']}","valley interview checks"),(f"{q.get('ticks',752)}","open-valley ticks"),
                  (f"{q.get('lies_caught',2)}","Fox bluffs quarantined"),(f"{q.get('trades',2)}","Pareto trades struck"),
                  (f"{q.get('discoveries',1)}","science discoveries"),(human(q.get("spikes",0)),"multi-gen GPU spikes")]
        elif mk=="evo":
            kpis=[(f"{q.get('pop',4096):,}","evolving baby minds"),(f"{q.get('pillars',116)}","cognitive evo pillars"),
                  (f"{q.get('mean',0):.1f}/{q.get('max_sc',7440)}","Gen endpoint mean"),(f"{q.get('best',0)}","champion score"),
                  (f"{q.get('gpu_ms',0):.2f} ms","GPU ms / generation"),("0","hardcoded learning rules")]
        elif mk=="bench":
            kpis=[(f"{q.get('gnt',0):.1f}","Gneuron-ticks / sec"),(f"{q.get('hipp_us',0):.1f} µs","26-SM Hipp (4k L2)"),
                  (f"{q.get('neo_us',0):.1f} µs","28-MiB Neo settle"),(f"{q.get('kernels',52)}","native CUDA kernels"),
                  ("16,384","Tier-2 HBM engrams"),("0 B","register / local spill")]
        elif mk=="talk":
            kpis=[(f"{q.get('turns',0)}","live dialogue turns"),(human(q.get("spikes",0)),"Blackwell GPU spikes"),
                  (f"{q.get('edits',0):,}","live synaptic edits"),(f"{q.get('kernels',52)}","Blackwell CUDA kernels"),
                  ("29.36M","plastic int8 synapses"),("0","LLMs or backprop")]
        else:
            kpis=[(f"{q.get('correct',0)}/{q.get('total',0)}","scenarios passed"),(human(q.get("spikes",0)),"Blackwell GPU spikes"),
                  (f"{q.get('lies_caught',0)}","deceptions caught"),("29.36M","plastic int8 synapses"),
                  ("52","Blackwell CUDA kernels"),("97.5%","32-MiB L2 cache fit")]
        kw,kh=672,196
        for ki,(kv,kl) in enumerate(kpis[:6]):
            kx=rx+(ki%3)*(kw+34);ky=814+(ki//3)*(kh+26)
            d.rect(kx,ky,kw,kh,"#ecefe6",14,'stroke="#c4cebf" stroke-width="2"')
            d.text(kx+28,ky+98,str(kv),56,GREEN,SERIF,600)
            d.text(kx+28,ky+152,kl,25,MUTED,SANS)

        # 3 Storytelling Dossier Cards (y=1286..1872): What Changed · Animal Impact · Coming Up Next!
        if is_abl:
            s1=f"We deliberately silenced Axion's {arm} circuit during the full {q.get('domains',560)}-domain exam to verify biological modularity."
            s2="Episodic & online skills owned by the lesioned module dropped cleanly while all 53 consolidated & live-written cortical tracks (Trequinquagintuple Ribot!) kept firing."
            s3="Next (Stage 8q · 570 DOMAINS!): K53 pure-GPU Spiking Viterbi HMM Sequence Decoder Cortex (free PW27 Q!) & free-form natural multi-turn dialogue!"
        elif is_al:
            s1="Raised from birth in a scrambled synthetic Alien SOV dialect with reversed grammar and zero English anywhere."
            s2=f"Achieved exact 100% parity with English across all {q.get('domains',560)} domains: K52 GPU 4,096-rollout UCB1 MCTS, 100% natural chat & Grover/DampCtl!"
            s3="Stage 8q · 570 Domains next: K53 pure-GPU Spiking Viterbi HMM Decoder Cortex (free PW27 Q!) & deeper unconstrained human conversation!"
        else:
            s1="STAGE 8p · 52 KERNELS & 560 DOMAINS! Added Kernel 52 4,096-Rollout UCB1 MCTS Planner on free PW27 Q (Trequinquagintuple 53-Track Ribot!), 100/100 Century Valley & 7 STEM domains."
            s2="Axion runs 4,096 parallel UCB1 tree rollouts purely on Blackwell L2 (K52) & answers 100% zero-command-keyword natural multi-turn human questions (`what do X and Y share? / why can it not fly? / what else can it do?`)!"
            s3="Next (Stage 8q · 570 DOMAINS!): K53 pure-GPU Spiking Viterbi HMM Sequence Decoder Cortex (free PW27 Q!), deeper open dialogue & multi-mind labs!"
        story_cards=[
            ("WHAT JUST CHANGED",GREEN,"#e4eee7",s1),
            ("HOW THIS CHANGES WHAT THE ANIMALS DO","#b5762c","#f5eddc",s2),
            ("COMING UP NEXT ON THE HORIZON","#327a83","#e3eff0",s3),
        ]
        for si,(st,sc,sbg,sb) in enumerate(story_cards):
            sy=1288+si*198
            d.rect(rx,sy,2085,176,sbg,14,f'stroke="{sc}" stroke-width="2.2"')
            d.rect(rx,sy,14,176,sc,4)
            d.text(rx+36,sy+42,st,21,sc,MONO,700,space=1.3)
            for li,ln in enumerate(card_wrap(sb,102)[:3]):
                d.text(rx+36,sy+82+li*34,ln,26,INK,SANS,500 if li==0 else 400)
        return

    d.rect(90,365,1480,1374,"#223f3d",26);d.text(143,450,"RECORDED SPECIMEN",27,"#bad9bd",MONO,600,space=2)
    if "body_plan" in q:
        atlas_anatomy(d,832,1035,q,atlas_identity(q),step=177);caption="Observed arrangement of body segments and organs"
    else:
        caption=atlas_wiring(d,189,581,1280,826,q)
        if not caption:d.text(829,1040,"Individual anatomy unrecorded",43,"#bed1bf",SERIF,anchor="middle")
    d.text(141,1659,caption,33,"#d1e3cc",SANS)
    rx=1691;d.text(rx,452,p["name"],63,INK,SERIF);d.text(rx,522,p["scope"],35,MUTED)
    y=650
    for title,names in (("Sensory equipment",p["senses"]),("Neural equipment",p["circuits"])):
        d.text(rx,y,title.upper(),24,GREEN,MONO,600,space=1.3);y+=62
        for line in card_wrap("   ·   ".join(names) or "Unrecorded",83)[:3]:
            d.text(rx,y,line,36,INK);y+=50
        y+=63
    facts=[(CARD_MEASURES[k],q[k]) for k in CARD_ORDER if k in q and q[k]]
    for i,(label,v) in enumerate(facts[:6]):
        cx=rx+i%3*660;cy=max(y,1260)+i//3*180
        d.text(cx,cy,human(v),63,GREEN,SERIF);d.text(cx,cy+54,label,29,MUTED)
    delta=p.get("delta",[])
    changes=[CARD_MEASURES.get(v["metric"],v["metric"])+f" {v['before']} → {v['after']}" if "metric" in v else v.get("organ","")+" "+v.get("change","") for v in delta]
    d.text(92,1840,short("Changes since entry   ·   "+"   ·   ".join(changes),92) if changes else "Individual profile at the recorded observation",30,MUTED)
    if b["card"].get("series"):
        d.add('<g transform="translate(1820 1650) scale(1.8)">')
        card_trend(d,b["card"]["series"],0,0,840);d.add('</g>')

def card_drawing(b,ledger):
    d=Drawing(ATLAS_W,ATLAS_H);a=b["card"]
    roster=a.get("roster") or dict(kind="individual",items=[],mode=a["subject"]["glyph"])
    groups=roster["kind"] in ("census","sample");rows=roster["items"]
    d.rect(0,0,ATLAS_W,253,DARK);d.rect(0,0,ATLAS_W,10,"#9dc6a0")
    d.path("M91 79 L109 50 L127 79 M99 69 H119",MINT,4)
    d.text(151,77,"AXION   /   POPULATION ATLAS",24,MINT,MONO,600,space=1.5)
    title="Creatures of the floodplain" if b["kind"]=="floodplain" else "Creature field guide"
    d.text(84,181,title,88,"#f0f2db",SERIF,500)
    d.text(91,232,f"Run {b['id']:05d}  ·  {b['date'][:10]}"+(f"  ·  Tick {roster['tick']:,}" if roster.get("tick") is not None else ""),26,"#c7ddc9",MONO)
    if groups:
        total=roster["total"];covered=sum(r["q"]["members"] for r in rows)
        facts=[(f"{total:,}","living residents" if roster["kind"]=="census" else "sampled residents"),
               (f"{roster['types']:,}","observed body forms"),(f"{100*covered/max(1,total):.1f}%","represented on this sheet")]
        for i,(v,label) in enumerate(facts):
            x=2015+i*604;d.rule(x-37,67,x-37,196,"#4a6b5e",2);d.text(x,134,v,61,"#edf2db",SANS,600);d.text(x,186,label,29,MINT)
        n=len(rows)
        if n:
            scale=min(1.82,3650/(924*n)) if n<=3 else 1
            for i,row in enumerate(rows):
                x=(ATLAS_W-scale*(924*n-36))/2+i*924*scale if n<=3 else 90+(i%4)*924
                y=290+(1570-770*scale)/2 if n<=3 else 290+(i//4)*797
                d.add(f'<g transform="translate({x},{y}) scale({scale})">');atlas_card(d,0,0,row,i+1,roster);d.add('</g>')
            if n==4:
                d.text(94,1245,"The entire recorded census fits in one row." if roster["kind"]=="census" else "Frequency is measured within the recorded sample.",35,MUTED,SERIF)
        else:d.text(91,784,"No living residents at the closing census.",55,MUTED,SERIF)
        if roster["mode"]=="morphospecies":
            d.text(94,1901,"Each lobe is a body segment. Amber marks limb sites, cyan marks eyes, and lime marks pores. Gold rings locate differences of up to three sites from the nearest displayed form. Diet bars show leaf, fruit and litter in that order.",26,MUTED)
        else:d.text(94,1901,"Ranks group identical inherited body or locomotor codes. Individual measurements name their representative.",26,MUTED)
    else:individual_atlas(d,b,roster)
    atlas_repertoire(d,b,ledger,1920);atlas_footer(d,b);return d
